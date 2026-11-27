#include <kos.h>

#include <stdint.h>
#include <stdio.h>

#include "dreampsx/audio_buffer.h"
#include "dreampsx/dreamcast_audio_stream.h"
#include "dreampsx/kos_audio_backend.h"

#define TONE_FRAMES_PER_TICK 441u
#define TONE_FREQUENCY_HZ 440u
#define OUTPUT_RATE_HZ 44100u
#define TONE_AMPLITUDE 7000

static PsxAudioBuffer audio_queue;
static PsxDreamcastAudioStream audio_stream;
static PsxKosAudioBackend audio_backend;

static void generate_tone(int16_t *samples, size_t frames, uint32_t *phase)
{
    size_t i;

    for (i = 0u; i < frames; ++i) {
        int16_t sample;
        *phase += TONE_FREQUENCY_HZ;
        if (*phase >= OUTPUT_RATE_HZ) *phase -= OUTPUT_RATE_HZ;
        sample = *phase < (OUTPUT_RATE_HZ / 2u)
            ? (int16_t)TONE_AMPLITUDE
            : (int16_t)-TONE_AMPLITUDE;
        samples[i * 2u] = sample;
        samples[i * 2u + 1u] = sample;
    }
}

int main(int argc, char **argv)
{
    uint32_t phase = 0u;
    uint32_t ticks_since_report = 0u;
    int16_t samples[TONE_FRAMES_PER_TICK * PSX_AUDIO_CHANNEL_COUNT];
    size_t prefill_remaining = PSX_AUDIO_BUFFER_CAPACITY_FRAMES;

    (void)argc;
    (void)argv;

    dbgio_dev_select("fb");
    printf("DreamPSX AICA audio smoke test\n");
    printf("Expected output: 440 Hz stereo tone\n");

    if (!psx_audio_buffer_init(&audio_queue) ||
        !psx_dreamcast_audio_stream_init(&audio_stream, &audio_queue)) {
        printf("ERROR: audio queue initialization failed\n");
        return 1;
    }

    /* Prime the PCM queue before snd_stream_start() requests its first block. */
    while (prefill_remaining != 0u) {
        size_t frames = prefill_remaining;
        size_t accepted;
        if (frames > TONE_FRAMES_PER_TICK) frames = TONE_FRAMES_PER_TICK;
        generate_tone(samples, frames, &phase);
        accepted = psx_audio_buffer_write(&audio_queue, samples, frames);
        if (accepted != frames) {
            printf("ERROR: initial PCM queue prefill failed\n");
            return 1;
        }
        prefill_remaining -= frames;
    }

    if (!psx_kos_audio_backend_start(&audio_backend, &audio_stream)) {
        printf("ERROR: audio backend initialization failed\n");
        return 1;
    }

    printf("AICA stream started: 44100 Hz, 16-bit stereo\n");
    printf("PCM prefill: %u frames (about 93 ms)\n",
           (unsigned)PSX_AUDIO_BUFFER_CAPACITY_FRAMES);

    for (;;) {
        size_t accepted;

        generate_tone(samples, TONE_FRAMES_PER_TICK, &phase);
        accepted = psx_audio_buffer_write(&audio_queue, samples,
                                          TONE_FRAMES_PER_TICK);
        if (accepted != TONE_FRAMES_PER_TICK) {
            printf("WARNING: audio queue overflow (%lu frames dropped)\n",
                   (unsigned long)(TONE_FRAMES_PER_TICK - accepted));
        }

        if (!psx_kos_audio_backend_poll(&audio_backend)) {
            printf("ERROR: AICA stream poll failed\n");
            psx_kos_audio_backend_stop(&audio_backend);
            return 1;
        }

        /* 441 frames at 44.1 kHz correspond to a 10 ms production interval. */
        thd_sleep(10);

        /* Keep diagnostics coarse: frequent framebuffer output can disturb pacing. */
        if (++ticks_since_report >= 500u) {
            printf("AUDIO: callbacks=%llu silence=%llu underrun=%llu "
                   "dropped=%llu queued=%lu\n",
                   (unsigned long long)audio_stream.callback_count,
                   (unsigned long long)audio_stream.silence_frames,
                   (unsigned long long)audio_queue.underrun_frames,
                   (unsigned long long)audio_queue.dropped_frames,
                   (unsigned long)audio_queue.frame_count);
            ticks_since_report = 0u;
        }
    }
}
