# Dreamcast audio stream callback bridge — milestone 4.42

## Goal

Prepare a bounded handoff from DreamPSX machine PCM output to the KallistiOS AICA streaming API without making the core depend on KallistiOS headers.

## KallistiOS contract reviewed

The current KallistiOS `dc/sound/stream.h` defines `snd_stream_callback_t` as a callback receiving a stream handle, requested sample count and output count pointer, and returning a sample buffer. Its documentation specifies interleaved samples for stereo and recommends 32-byte alignment. The stream API provides `snd_stream_start()` for 16-bit PCM and `snd_stream_poll()`, which must be called periodically to keep streaming output supplied.

Sources:
- https://kos-docs.dreamcast.wiki/stream_8h.html
- https://raw.githubusercontent.com/KallistiOS/KallistiOS/master/kernel/arch/dreamcast/include/dc/sound/stream.h

## Implementation

- `PsxDreamcastAudioStream` binds to the machine-owned `PsxAudioBuffer`.
- Its fixed callback scratch buffer is 32-byte aligned and holds up to 1024 stereo frames.
- The callback returns interleaved signed 16-bit PCM and reports the number of frames supplied.
- Short reads are zero-filled, so no stale samples escape into the AICA stream.
- Oversized requests are safely clamped and counted; invalid arguments are rejected.
- The adapter is independent of KallistiOS and is testable in the host CI.

## Validation

The new host test checks alignment, stereo order, queue consumption, zero-fill on underrun, oversized requests, telemetry, invalid arguments and reset behavior.

## Limitations

This milestone does not yet call `snd_stream_init`, allocate or start a KallistiOS stream, or call `snd_stream_poll`. It is the host-tested callback payload adapter, not a compiled KallistiOS backend. The KallistiOS toolchain and Dreamcast hardware/Flycast playback have not been validated. The adapter is single-threaded; concurrency between emulation and audio callbacks still requires a safe producer/consumer design before enabling asynchronous playback.
