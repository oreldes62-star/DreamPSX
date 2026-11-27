#ifndef DREAMPSX_AUDIO_BUFFER_H
#define DREAMPSX_AUDIO_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSX_AUDIO_BUFFER_CAPACITY_FRAMES 4096u
#define PSX_AUDIO_CHANNEL_COUNT 2u

typedef struct {
    int16_t samples[PSX_AUDIO_BUFFER_CAPACITY_FRAMES * PSX_AUDIO_CHANNEL_COUNT];
    uint32_t read_index;
    uint32_t write_index;
    uint32_t frame_count;
    uint64_t dropped_frames;
    uint64_t underrun_frames;
    bool initialized;
} PsxAudioBuffer;

/* Bounded interleaved stereo queue. Single-threaded access only for now. */
bool psx_audio_buffer_init(PsxAudioBuffer *buffer);
void psx_audio_buffer_reset(PsxAudioBuffer *buffer);
/* Writes as many frames as fit and returns the number accepted. */
size_t psx_audio_buffer_write(PsxAudioBuffer *buffer,
                              const int16_t *interleaved_stereo,
                              size_t frame_count);
/* Reads available frames, zero-fills any shortfall, and returns frames read. */
size_t psx_audio_buffer_read(PsxAudioBuffer *buffer,
                             int16_t *interleaved_stereo,
                             size_t frame_capacity);

#endif
