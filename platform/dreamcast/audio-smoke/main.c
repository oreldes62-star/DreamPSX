#include <kos.h>

#include <stdint.h>
#include <stdio.h>

#include "dreampsx/audio_buffer.h"
#include "dreampsx/dreamcast_audio_stream.h"
#include "dreampsx/kos_audio_backend.h"

#define TONE_FRAMES_PER_TICK 44u
#define TONE_FREQUENCY_HZ 440u
#define OUTPUT_RATE_HZ 44100u
#define TONE_AMPLITUDE 7000

static PsxAudioBuffer audio_queue;
static PsxDreamcastAudioStream audio_stream;
static PsxKosAudioBackend audio_backend;

int main(int argc, char **argv)
{
    uint32_t phase = 0u;
    int16_t samples[TONE_FRAMES_PER_TICK * PSX_AUDIO_CHANNEL_COUNT];

    (void)argc;
    (void)argv;

    dbgio_dev_select("fb");
    printf("DreamPSX AICA audio smoke test\n");
    printf("Expected output: 440 Hz stereo tone\n");

    if (!psx_audio_buffer_init(&audio_queue) ||
        !psx_dreamcast_audio_stream_init(&audio_stream, &audio_queue) ||
        !psx_kos_audio_backend_start(&audio_backend, &audio_stream)) {
        printf("ERROR: audio backend initialization failed\n");
        return 1;
    }

    printf("AICA stream started: 44100 Hz, 16-bit stereo\n");

    for (;;) {
        size_t i;
        size_t accepted;

        for (i = 0u; i < TONE_FRAMES_PER_TICK; ++i) {
            int16_t sample;
            phase += TONE_FREQUENCY_HZ;
            if (phase >= OUTPUT_RATE_HZ) phase -= OUTPUT_RATE_HZ;
            sample = phase < (OUTPUT_RATE_HZ / 2u)
                ? (int16_t)TONE_AMPLITUDE
                : (int16_t)-TONE_AMPLITUDE;
            samples[i * 2u] = sample;
            samples[i * 2u + 1u] = sample;
        }

        accepted = psx_audio_buffer_write(&audio_queue, samples,
                                          TONE_FRAMES_PER_TICK);
        if (accepted != TONE_FRAMES_PER_TICK) {
            printf("WARNING: audio queue overflow (%lu frames dropped)\n",
                   (unsigned long)audio_queue.dropped_frames);
        }

        if (!psx_kos_audio_backend_poll(&audio_backend)) {
            printf("ERROR: AICA stream poll failed\n");
            psx_kos_audio_backend_stop(&audio_backend);
            return 1;
        }

        thd_sleep(1);
    }
}
