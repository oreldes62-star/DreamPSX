#ifndef DREAMPSX_DREAMCAST_AUDIO_STREAM_H
#define DREAMPSX_DREAMCAST_AUDIO_STREAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "dreampsx/audio_buffer.h"

/* KallistiOS callback requests are byte counts for interleaved stereo PCM16.
 * Keep the callback scratch area bounded and 32-byte aligned.
 */
#define PSX_DREAMCAST_AUDIO_CALLBACK_MAX_FRAMES 1024u

typedef struct {
    PsxAudioBuffer *source;
    _Alignas(32) int16_t pcm[PSX_DREAMCAST_AUDIO_CALLBACK_MAX_FRAMES *
                             PSX_AUDIO_CHANNEL_COUNT];
    uint64_t callback_count;
    uint64_t silence_frames;
    uint64_t oversized_requests;
    bool initialized;
} PsxDreamcastAudioStream;

/* Initialize a host-testable bridge from the machine PCM queue to the
 * KallistiOS interleaved-stream callback contract.
 */
bool psx_dreamcast_audio_stream_init(PsxDreamcastAudioStream *stream,
                                     PsxAudioBuffer *source);
void psx_dreamcast_audio_stream_reset(PsxDreamcastAudioStream *stream);
/* Matches KallistiOS snd_stream_callback_t units: requested/received are
 * total interleaved bytes, not frames. The source queue remains frame-based.
 * Short source reads are completed with silence; partial stereo PCM16 frames
 * are never returned.
 */
void *psx_dreamcast_audio_stream_callback(PsxDreamcastAudioStream *stream,
                                          int bytes_requested,
                                          int *bytes_received);

#endif
