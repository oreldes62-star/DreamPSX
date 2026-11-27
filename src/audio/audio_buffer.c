#include "dreampsx/audio_buffer.h"

#include <string.h>

bool psx_audio_buffer_init(PsxAudioBuffer *buffer)
{
    if (!buffer) return false;
    memset(buffer, 0, sizeof(*buffer));
    buffer->initialized = true;
    return true;
}

void psx_audio_buffer_reset(PsxAudioBuffer *buffer)
{
    if (!buffer || !buffer->initialized) return;
    buffer->read_index = 0u;
    buffer->write_index = 0u;
    buffer->frame_count = 0u;
    buffer->dropped_frames = 0u;
    buffer->underrun_frames = 0u;
    memset(buffer->samples, 0, sizeof(buffer->samples));
}

size_t psx_audio_buffer_write(PsxAudioBuffer *buffer,
                              const int16_t *interleaved_stereo,
                              size_t frame_count)
{
    size_t accepted;
    if (!buffer || !buffer->initialized ||
        (!interleaved_stereo && frame_count != 0u))
        return 0u;

    accepted = frame_count;
    if (accepted > PSX_AUDIO_BUFFER_CAPACITY_FRAMES - buffer->frame_count)
        accepted = PSX_AUDIO_BUFFER_CAPACITY_FRAMES - buffer->frame_count;

    for (size_t i = 0u; i < accepted; ++i) {
        const uint32_t slot = (buffer->write_index + (uint32_t)i) %
                             PSX_AUDIO_BUFFER_CAPACITY_FRAMES;
        buffer->samples[slot * 2u] = interleaved_stereo[i * 2u];
        buffer->samples[slot * 2u + 1u] = interleaved_stereo[i * 2u + 1u];
    }
    buffer->write_index = (buffer->write_index + (uint32_t)accepted) %
                          PSX_AUDIO_BUFFER_CAPACITY_FRAMES;
    buffer->frame_count += (uint32_t)accepted;
    buffer->dropped_frames += (uint64_t)(frame_count - accepted);
    return accepted;
}

size_t psx_audio_buffer_read(PsxAudioBuffer *buffer,
                             int16_t *interleaved_stereo,
                             size_t frame_capacity)
{
    size_t available;
    if (!buffer || !buffer->initialized ||
        (!interleaved_stereo && frame_capacity != 0u))
        return 0u;

    available = frame_capacity;
    if (available > buffer->frame_count)
        available = buffer->frame_count;

    for (size_t i = 0u; i < available; ++i) {
        const uint32_t slot = (buffer->read_index + (uint32_t)i) %
                             PSX_AUDIO_BUFFER_CAPACITY_FRAMES;
        interleaved_stereo[i * 2u] = buffer->samples[slot * 2u];
        interleaved_stereo[i * 2u + 1u] = buffer->samples[slot * 2u + 1u];
    }
    for (size_t i = available; i < frame_capacity; ++i) {
        interleaved_stereo[i * 2u] = 0;
        interleaved_stereo[i * 2u + 1u] = 0;
    }

    buffer->read_index = (buffer->read_index + (uint32_t)available) %
                         PSX_AUDIO_BUFFER_CAPACITY_FRAMES;
    buffer->frame_count -= (uint32_t)available;
    buffer->underrun_frames += (uint64_t)(frame_capacity - available);
    return available;
}
