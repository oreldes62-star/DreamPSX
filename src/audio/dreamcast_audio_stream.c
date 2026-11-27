#include "dreampsx/dreamcast_audio_stream.h"

#include <string.h>

bool psx_dreamcast_audio_stream_init(PsxDreamcastAudioStream *stream,
                                     PsxAudioBuffer *source)
{
    if (!stream || !source || !source->initialized) return false;
    memset(stream, 0, sizeof(*stream));
    stream->source = source;
    stream->initialized = true;
    return true;
}

void psx_dreamcast_audio_stream_reset(PsxDreamcastAudioStream *stream)
{
    if (!stream || !stream->initialized) return;
    memset(stream->pcm, 0, sizeof(stream->pcm));
    stream->callback_count = 0u;
    stream->silence_frames = 0u;
    stream->oversized_requests = 0u;
}

void *psx_dreamcast_audio_stream_callback(PsxDreamcastAudioStream *stream,
                                          int samples_requested,
                                          int *samples_received)
{
    size_t requested;
    size_t available;
    if (samples_received) *samples_received = 0;
    if (!stream || !stream->initialized || !stream->source ||
        !stream->source->initialized || !samples_received ||
        samples_requested < 0)
        return NULL;

    requested = (size_t)samples_requested;
    if (requested > PSX_DREAMCAST_AUDIO_CALLBACK_MAX_FRAMES) {
        requested = PSX_DREAMCAST_AUDIO_CALLBACK_MAX_FRAMES;
        stream->oversized_requests++;
    }

    available = psx_audio_buffer_read(stream->source, stream->pcm, requested);
    stream->callback_count++;
    stream->silence_frames += (uint64_t)(requested - available);
    *samples_received = (int)requested;
    return stream->pcm;
}
