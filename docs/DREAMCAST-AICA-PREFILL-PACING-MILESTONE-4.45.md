# Dreamcast AICA audio prefill and pacing — milestone 4.45

## Change

The standalone KallistiOS audio diagnostic now primes the bounded PCM queue before starting the AICA stream. This prevents the stream's initial buffer-fill callbacks from receiving only silence because the producer has not yet run.

The diagnostic then produces 441 stereo frames per 10 ms tick at 44.1 kHz. The 4096-frame initial queue represents approximately 92.9 ms of audio headroom.

## Scope

- The change affects only the standalone audio smoke application.
- The 440 Hz square-wave signal remains a diagnostic tone, not emulated PlayStation audio.
- Queue overflow is still reported; actual callback underruns remain measurable in the stream and source queue counters.
- The core emulator is cross-compiled by the existing KallistiOS workflow, but this milestone does not claim that a CDI image or game is runnable.

## Cross-build validation

The updated application was successfully built with the KallistiOS SH-4 toolchain. The workflow verified the ELF32 little-endian SuperH executable and the 23-member DreamPSX core archive:
https://github.com/oreldes62-star/DreamPSX/actions/runs/46069353152

Artifact:
https://github.com/oreldes62-star/DreamPSX/actions/runs/46069353152/artifacts/15232685721

## Runtime validation still required

The resulting ELF must still be launched in Flycast and, later, on physical Dreamcast hardware to verify audible output and sustained playback without underruns.
