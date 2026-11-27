# Dreamcast AICA audio prefill and pacing — milestone 4.45

## Change

The standalone KallistiOS audio diagnostic now primes the bounded PCM queue before starting the AICA stream. This prevents the stream's initial buffer-fill callbacks from receiving only silence because the producer has not yet run.

The diagnostic then produces 441 stereo frames per 10 ms tick at 44.1 kHz. The 4096-frame initial queue represents approximately 92.9 ms of audio headroom.

## Scope

- The change affects only the standalone audio smoke application.
- The 440 Hz square-wave signal remains a diagnostic tone, not emulated PlayStation audio.
- Queue overflow is still reported; actual callback underruns remain measurable in the stream and source queue counters.
- The core emulator is cross-compiled by the existing KallistiOS workflow, but this milestone does not claim that a CDI image or game is runnable.

## Validation still required

The KallistiOS CI workflow must compile the updated smoke application. The resulting ELF must then be launched in Flycast and, later, on physical Dreamcast hardware to verify audible output and sustained playback without underruns.
