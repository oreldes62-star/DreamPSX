# Dreamcast AICA byte-count callback integration — milestone 4.57

## Goal

Make the host-tested PCM callback obey the exact data-size contract used by the pinned KallistiOS AICA stream implementation, and verify the machine SPU output reaches that callback intact.

## KallistiOS contract

The pinned KallistiOS revision is `9926b70aeafc8398675caef98813f10d15d8a329`.

In `kernel/arch/dreamcast/sound/snd_stream.c`, `snd_stream_fill()` computes `needed_bytes = size * channels`, calls the user callback with that byte count, and consumes the callback's `got_bytes` result as bytes. For stereo 16-bit PCM, one frame is four interleaved bytes (left sample followed by right sample).

This differs from the internal DreamPSX audio queue, whose read/write API is intentionally expressed in stereo frames.

Source:
- https://raw.githubusercontent.com/KallistiOS/KallistiOS/9926b70aeafc8398675caef98813f10d15d8a329/kernel/arch/dreamcast/sound/snd_stream.c
- https://raw.githubusercontent.com/KallistiOS/KallistiOS/9926b70aeafc8398675caef98813f10d15d8a329/kernel/arch/dreamcast/include/dc/sound/stream.h

## Correction

The callback adapter previously treated KallistiOS' requested byte count as a frame count. That could cause the callback to report too few bytes to KallistiOS, resulting in an underfilled AICA stream even though the code compiled.

The adapter now:
- interprets the KallistiOS request and return count as total interleaved bytes;
- converts only complete four-byte stereo PCM16 frames to queue operations;
- clamps oversized requests to its 32-byte-aligned scratch buffer;
- zero-fills queue underruns and reports the actual complete-frame byte count;
- keeps queue telemetry expressed in frames.

The backend's `snd_stream_alloc()` size remains 2048 bytes per channel. With stereo enabled, KallistiOS requests 2048 interleaved bytes for each half-buffer fill, which corresponds to 512 stereo frames and fits the callback scratch capacity.

## End-to-end host coverage

The machine integration test now initializes the machine SPU with a deterministic active voice, renders PCM into `machine.audio_output`, then retrieves it through `PsxDreamcastAudioStream`. It verifies:
- left/right samples remain interleaved;
- the callback reports 16 bytes for four stereo PCM16 frames;
- the queue is consumed exactly once;
- no silence is counted when all four frames are available.

The dedicated callback tests also cover byte counts, partial-frame rounding, oversized requests, zero-fill, alignment and invalid arguments.

## Validation

- Host CMake build and CTest: 44/44 passed after callback correction and machine-to-callback integration (GitHub Actions run 46077164459).
- The earlier KallistiOS SH-4 build after the byte-count correction passed, compiling the core archive, linking the audio smoke ELF, verifying ELF32 little-endian SuperH and uploading artifact ID 15234764907 (run 46076903036).
- A further KallistiOS build for the latest integration commit is running separately; its result must be checked before claiming that exact head has cross-build validation.

## Limitations

The Dreamcast application still emits a diagnostic 440 Hz tone rather than booting a PlayStation game. The corrected AICA callback has not yet been auditioned in Flycast or on physical Dreamcast hardware. This milestone does not claim CDI packaging, asynchronous thread safety, real-time performance, or game compatibility.
