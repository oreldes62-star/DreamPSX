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

The source change is committed. It still requires the Dreamcast KallistiOS CI build, followed by execution in Flycast and physical-hardware listening tests. The diagnostic 440 Hz tone is not yet evidence that PlayStation SPU audio is fully integrated or that games are playable.
