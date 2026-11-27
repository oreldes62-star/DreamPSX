#include "dreampsx/kos_audio_backend.h"

#include <dc/sound/stream.h>

static void *dreampsx_stream_callback(snd_stream_hnd_t handle,
                                      int samples_requested,
                                      int *samples_received)
{
    PsxDreamcastAudioStream *stream =
        (PsxDreamcastAudioStream *)snd_stream_get_userdata(handle);
    return psx_dreamcast_audio_stream_callback(stream, samples_requested,
                                                samples_received);
}

bool psx_kos_audio_backend_start(PsxKosAudioBackend *backend,
                                 PsxDreamcastAudioStream *stream)
{
    snd_stream_hnd_t handle;
    if (!backend) return false;

    /* Leave a safely stoppable state even when validation fails. */
    backend->stream_handle = SND_STREAM_INVALID;
    backend->stream_system_initialized = false;
    backend->initialized = false;
    if (!stream || !stream->initialized) return false;

    /* KOS documents that stream_init initializes the sound subsystem too. */
    if (snd_stream_init() != 0) return false;
    backend->stream_system_initialized = true;

    handle = snd_stream_alloc(dreampsx_stream_callback,
        (int)(PSX_DREAMCAST_AUDIO_CALLBACK_MAX_FRAMES * sizeof(int16_t)));
    if (handle == SND_STREAM_INVALID) {
        snd_stream_shutdown();
        backend->stream_system_initialized = false;
        return false;
    }

    backend->stream_handle = handle;
    snd_stream_set_userdata(handle, stream);
    snd_stream_start(handle, 44100u, 1);
    backend->initialized = true;
    return true;
}

bool psx_kos_audio_backend_poll(PsxKosAudioBackend *backend)
{
    if (!backend || !backend->initialized ||
        backend->stream_handle == SND_STREAM_INVALID)
        return false;
    return snd_stream_poll(backend->stream_handle) == 0;
}

void psx_kos_audio_backend_stop(PsxKosAudioBackend *backend)
{
    if (!backend) return;
    if (backend->initialized &&
        backend->stream_handle != SND_STREAM_INVALID) {
        snd_stream_stop(backend->stream_handle);
        snd_stream_destroy(backend->stream_handle);
    }
    if (backend->stream_system_initialized) snd_stream_shutdown();
    backend->stream_handle = SND_STREAM_INVALID;
    backend->stream_system_initialized = false;
    backend->initialized = false;
}
