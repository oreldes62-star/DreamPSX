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
                                          int bytes_requested,
                                          int *bytes_received)
{
    const size_t bytes_per_frame =
        PSX_AUDIO_CHANNEL_COUNT * sizeof(int16_t);
    size_t requested_bytes;
    size_t requested_frames;
    size_t available;

    if (bytes_received) *bytes_received = 0;
    if (!stream || !stream->initialized || !stream->source ||
        !stream->source->initialized || !bytes_received ||
        bytes_requested < 0)
        return NULL;

    /* KallistiOS snd_stream_fill() passes the total interleaved byte count
     * to its callback and expects the returned count in bytes as well.
     * The queue itself remains frame-based (one L/R pair per frame).
     */
    requested_bytes = (size_t)bytes_requested;
    if (requested_bytes >
        sizeof(stream->pcm)) {
        requested_bytes = sizeof(stream->pcm);
        stream->oversized_requests++;
    }

    /* A stereo PCM16 frame is four bytes. Never expose a partial frame. */
    requested_bytes -= requested_bytes % bytes_per_frame;
    requested_frames = requested_bytes / bytes_per_frame;
    available = psx_audio_buffer_read(stream->source, stream->pcm,
                                      requested_frames);
    stream->callback_count++;
    stream->silence_frames += (uint64_t)(requested_frames - available);
    *bytes_received = (int)(requested_frames * bytes_per_frame);
    return stream->pcm;
}
