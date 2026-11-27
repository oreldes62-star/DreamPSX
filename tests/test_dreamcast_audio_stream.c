#include "dreampsx/dreamcast_audio_stream.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    PsxAudioBuffer queue;
    PsxDreamcastAudioStream stream;
    int16_t source[8] = {100, -100, 200, -200, 300, -300, 400, -400};
    int received = -1;
    int16_t *pcm;

    assert(psx_audio_buffer_init(&queue));
    assert(psx_dreamcast_audio_stream_init(&stream, &queue));
    assert(((uintptr_t)stream.pcm & 31u) == 0u);

    assert(psx_audio_buffer_write(&queue, source, 4u) == 4u);
    pcm = (int16_t *)psx_dreamcast_audio_stream_callback(&stream, 3, &received);
    assert(pcm != NULL && received == 3);
    assert(pcm[0] == 100 && pcm[1] == -100);
    assert(pcm[4] == 300 && pcm[5] == -300);
    assert(queue.frame_count == 1u);

    pcm = (int16_t *)psx_dreamcast_audio_stream_callback(&stream, 3, &received);
    assert(pcm != NULL && received == 3);
    assert(pcm[0] == 400 && pcm[1] == -400);
    assert(pcm[2] == 0 && pcm[3] == 0);
    assert(pcm[4] == 0 && pcm[5] == 0);
    assert(stream.silence_frames == 2u);
    assert(queue.underrun_frames == 2u);

    pcm = (int16_t *)psx_dreamcast_audio_stream_callback(
        &stream, (int)PSX_DREAMCAST_AUDIO_CALLBACK_MAX_FRAMES + 10, &received);
    assert(pcm != NULL);
    assert(received == (int)PSX_DREAMCAST_AUDIO_CALLBACK_MAX_FRAMES);
    assert(stream.oversized_requests == 1u);
    assert(stream.silence_frames ==
           2u + PSX_DREAMCAST_AUDIO_CALLBACK_MAX_FRAMES);
    assert(stream.callback_count == 3u);

    received = 123;
    assert(psx_dreamcast_audio_stream_callback(&stream, -1, &received) == NULL);
    assert(received == 0);
    assert(psx_dreamcast_audio_stream_callback(&stream, 1, NULL) == NULL);
    assert(!psx_dreamcast_audio_stream_init(NULL, &queue));
    assert(!psx_dreamcast_audio_stream_init(&stream, NULL));

    psx_dreamcast_audio_stream_reset(&stream);
    assert(stream.callback_count == 0u);
    assert(stream.silence_frames == 0u);
    assert(stream.oversized_requests == 0u);
    puts("Dreamcast audio stream bridge tests passed");
    return 0;
}
