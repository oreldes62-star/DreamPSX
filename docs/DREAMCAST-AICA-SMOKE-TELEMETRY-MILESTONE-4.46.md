# AICA smoke-test telemetry milestone 4.46

## Change

The standalone Dreamcast audio diagnostic now reports a compact health snapshot every five seconds:

- KallistiOS stream callback count;
- frames completed with silence by the stream bridge;
- PCM queue underrun frames;
- PCM frames dropped because the queue was full;
- current queued frame count.

The report interval is deliberately coarse so framebuffer logging does not run on every audio tick and unnecessarily perturb timing.

## Why this matters

A successful stream start alone does not prove stable audio. These counters help distinguish an output-path failure from producer pacing problems and expose whether the bounded queue is draining too quickly or filling up.

## Validation status

The source change is committed. The updated telemetry application has now compiled successfully with the KallistiOS SH-4 toolchain and passed ELF32/SuperH verification. The resulting build artifact is available here: https://github.com/oreldes62-star/DreamPSX/actions/runs/46069491488/artifacts/15232895515. Execution in Flycast and physical-hardware listening tests are still required. The diagnostic 440 Hz tone is not yet evidence that PlayStation SPU audio is fully integrated or that games are playable.
