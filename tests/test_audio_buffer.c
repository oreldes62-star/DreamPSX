#include "dreampsx/audio_buffer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    PsxAudioBuffer buffer;
    int16_t input[PSX_AUDIO_BUFFER_CAPACITY_FRAMES * 2u];
    int16_t output[PSX_AUDIO_BUFFER_CAPACITY_FRAMES * 2u + 8u];

    assert(psx_audio_buffer_init(&buffer));
    assert(buffer.frame_count == 0u);
    assert(psx_audio_buffer_write(&buffer, NULL, 0u) == 0u);
    assert(psx_audio_buffer_write(&buffer, NULL, 1u) == 0u);
    assert(psx_audio_buffer_read(&buffer, NULL, 1u) == 0u);
    assert(psx_audio_buffer_read(NULL, output, 1u) == 0u);
    assert(psx_audio_buffer_write(NULL, input, 1u) == 0u);
    assert(psx_audio_buffer_read(&buffer, output, 0u) == 0u);

    input[0] = 100;
    input[1] = -100;
    input[2] = 200;
    input[3] = -200;
    assert(psx_audio_buffer_write(&buffer, input, 2u) == 2u);
    assert(buffer.frame_count == 2u);
    memset(output, 0x7f, sizeof(output));
    assert(psx_audio_buffer_read(&buffer, output, 3u) == 2u);
    assert(output[0] == 100 && output[1] == -100);
    assert(output[2] == 200 && output[3] == -200);
    assert(output[4] == 0 && output[5] == 0);
    assert(buffer.underrun_frames == 1u);

    for (size_t i = 0u; i < PSX_AUDIO_BUFFER_CAPACITY_FRAMES * 2u; ++i)
        input[i] = (int16_t)i;
    assert(psx_audio_buffer_write(&buffer, input,
                                  PSX_AUDIO_BUFFER_CAPACITY_FRAMES) ==
           PSX_AUDIO_BUFFER_CAPACITY_FRAMES);
    assert(psx_audio_buffer_write(&buffer, input, 4u) == 0u);
    assert(buffer.dropped_frames == 4u);
    /* A rejected write must not overwrite already queued samples. */
    /* The previous two-frame read left the ring cursor at physical slot 2. */
    assert(buffer.samples[2u * 2u] == input[0]);
    assert(buffer.samples[2u * 2u + 1u] == input[1]);
    assert(buffer.samples[1u * 2u] ==
           input[(PSX_AUDIO_BUFFER_CAPACITY_FRAMES - 1u) * 2u]);

    /* Consume most frames, then write across the physical end of the ring. */
    assert(psx_audio_buffer_read(&buffer, output, 4090u) == 4090u);
    assert(psx_audio_buffer_write(&buffer, input, 6u) == 6u);
    assert(buffer.frame_count == 12u);
    assert(psx_audio_buffer_read(&buffer, output, 12u) == 12u);
    assert(output[0] == input[8180u]);
    assert(output[1] == input[8181u]);
    assert(output[12u] == input[0]);
    assert(output[13u] == input[1]);
    assert(buffer.frame_count == 0u);

    psx_audio_buffer_reset(&buffer);
    assert(buffer.frame_count == 0u);
    assert(buffer.dropped_frames == 0u);
    assert(buffer.underrun_frames == 0u);
    assert(psx_audio_buffer_read(&buffer, NULL, 0u) == 0u);
    assert(!psx_audio_buffer_init(NULL));
    puts("audio buffer tests passed");
    return 0;
}
