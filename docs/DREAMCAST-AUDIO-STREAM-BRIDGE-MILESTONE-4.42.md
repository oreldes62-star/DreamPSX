# Dreamcast audio stream callback bridge — milestone 4.42

## Goal

Prepare a bounded handoff from DreamPSX machine PCM output to the KallistiOS AICA streaming API without making the core depend on KallistiOS headers.

## KallistiOS contract reviewed

The pinned KallistiOS implementation calls the callback with the total interleaved byte count required by `snd_stream_fill()` and expects the returned count in bytes. For stereo PCM16, four bytes represent one L/R frame. The callback data is interleaved and the stream bridge's scratch buffer is 32-byte aligned. `snd_stream_poll()` must be called periodically to keep streaming output supplied.

Sources:
- https://kos-docs.dreamcast.wiki/stream_8h.html
- https://raw.githubusercontent.com/KallistiOS/KallistiOS/master/kernel/arch/dreamcast/include/dc/sound/stream.h

## Implementation

- `PsxDreamcastAudioStream` binds to the machine-owned `PsxAudioBuffer`.
- Its fixed callback scratch buffer is 32-byte aligned and holds up to 1024 stereo frames.
- The callback returns interleaved signed 16-bit PCM and reports the number of bytes supplied, matching the pinned KallistiOS implementation. Internally, the source queue is still read in stereo frames.
- Short reads are zero-filled, so no stale samples escape into the AICA stream.
- Oversized requests are safely clamped and counted; invalid arguments are rejected.
- The adapter is independent of KallistiOS and is testable in the host CI.

## Validation

The host tests check alignment, stereo order, byte-count units, complete-frame handling, queue consumption, zero-fill on underrun, oversized requests, telemetry, invalid arguments and reset behavior.

## Limitations

The byte-count contract was cross-checked against the pinned KallistiOS `snd_stream.c` implementation: `snd_stream_fill()` passes `needed_bytes` to the callback and consumes `got_bytes` as bytes. A later integration review corrected the original adapter's mistaken frame-count interpretation and added byte-level regression coverage. The adapter remains single-threaded; concurrency between emulation and audio callbacks still requires a safe producer/consumer design.

## Corrected integration validation

- Host CMake/CTest: 44/44 passed after byte-count correction (run 46076939118).
- KallistiOS SH-4 cross-build: all 23 core/platform translation units compiled, audio smoke ELF linked, ELF32 little-endian SuperH header verified and artifact uploaded (run 46076903036; artifact `dreampsx-dreamcast-build`, ID 15234764907).
- The artifact has not been run in Flycast or on physical Dreamcast hardware.
