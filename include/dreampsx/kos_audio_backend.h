#ifndef DREAMPSX_KOS_AUDIO_BACKEND_H
#define DREAMPSX_KOS_AUDIO_BACKEND_H
#include <stdbool.h>
#include "dreampsx/dreamcast_audio_stream.h"

/* KOS stream allocation size passed to snd_stream_alloc(). Callback payload
 * lengths are total interleaved bytes; this configured buffer yields 512
 * stereo PCM16 frames per 2048-byte callback block.
 */
#define PSX_KOS_AUDIO_STREAM_BUFFER_BYTES 2048u

typedef struct {
    int stream_handle;
    bool stream_system_initialized;
    bool initialized;
} PsxKosAudioBackend;

/* KallistiOS lifecycle wrapper. Build this source with KOS headers/library. */
bool psx_kos_audio_backend_start(PsxKosAudioBackend *backend,
                                 PsxDreamcastAudioStream *stream);
bool psx_kos_audio_backend_poll(PsxKosAudioBackend *backend);
void psx_kos_audio_backend_stop(PsxKosAudioBackend *backend);

#endif
