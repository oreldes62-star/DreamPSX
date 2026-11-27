# KallistiOS AICA stream lifecycle — milestone 4.43

## Scope

Adds the platform-specific KallistiOS wrapper around the host-tested PCM callback bridge from milestone 4.42.

## Backend behavior

- Calls `snd_stream_init()` to initialize the KallistiOS streaming subsystem.
- Allocates a 16-bit PCM stream with a bounded per-channel buffer.
- Associates the DreamPSX callback adapter through `snd_stream_set_userdata()`.
- Starts 44,100 Hz stereo PCM playback with `snd_stream_start()`.
- Exposes `psx_kos_audio_backend_poll()`; the application must call it regularly because KallistiOS documents that streams need periodic polling to keep producing audio.
- Stops and destroys the stream, then shuts down the streaming subsystem during cleanup.
- Handles initialization and stream-allocation failures without leaving the backend marked active.

## Validation

The host CI compiles the actual backend source against a test stub matching the documented KallistiOS `dc/sound/stream.h` API. The test drives initialization, stream configuration, callback delivery, poll errors and shutdown.

Latest host CI: 44/44 tests passed:
https://github.com/oreldes62-star/DreamPSX/actions/runs/46068480415

## Important limitations

The stub verifies C integration and lifecycle calls, not the real KallistiOS binary ABI or hardware behavior. The optional `DREAMPSX_BUILD_KOS_AUDIO_BACKEND` CMake target requires a Dreamcast cross-toolchain and a real `KOS_BASE` SDK include tree. That target has not yet been built against an installed KallistiOS SDK. No CDI, Flycast, or physical Dreamcast audio playback has been tested.

The application must call `psx_kos_audio_backend_poll()` frequently from its main loop. This is intentionally a synchronous polling design; it does not claim interrupt-safe or concurrent producer/consumer operation. The emulator's audio queue and the callback bridge are still single-threaded.
