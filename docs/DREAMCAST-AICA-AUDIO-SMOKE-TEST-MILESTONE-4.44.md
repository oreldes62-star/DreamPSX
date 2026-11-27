# Dreamcast AICA audio smoke application — milestone 4.44

## Purpose

A small KallistiOS application isolates the Dreamcast audio output path from PS1 emulation. It starts the DreamPSX PCM queue, callback adapter and KallistiOS AICA stream, then continuously feeds a 440 Hz square-wave test tone in stereo at 44,100 Hz.

This is a hardware-path diagnostic, not game audio and not proof of emulator performance.

## Build

Run from `platform/dreamcast/audio-smoke` after sourcing the KallistiOS environment:

```sh
. "$KOS_BASE/environ.sh"
make
```

At the initial 4.44 stage, the Makefile built only the audio queue, callback bridge, KallistiOS wrapper and smoke application. Milestone 4.46 subsequently extended that Makefile to cross-compile all 23 core/platform translation units into a Dreamcast static archive. The diagnostic application itself still emits a test tone and does not boot a PS1 game.

## Automated Dreamcast toolchain workflow

`.github/workflows/dreamcast-kos-audio.yml`:
- checks out DreamPSX and a pinned KallistiOS source revision;
- pulls the KallistiOS project's Dreamcast toolchain image;
- builds KallistiOS for Dreamcast;
- compiles the audio smoke-test ELF;
- verifies its ELF header and uploads the executable as a workflow artifact.

## Cross-compilation result

The real KallistiOS toolchain workflow completed successfully:
https://github.com/oreldes62-star/DreamPSX/actions/runs/46069028372

The resulting executable was inspected by the SH-ELF readelf tool and reported:
- ELF32
- little-endian
- executable file type
- Renesas / SuperH SH machine

The ELF was uploaded with the core archive as the `dreampsx-dreamcast-build` workflow artifact:
https://github.com/oreldes62-star/DreamPSX/actions/runs/46069353152/artifacts/15232685721

The host test suite also remains green at 44/44 tests:
https://github.com/oreldes62-star/DreamPSX/actions/runs/46069001718

## Runtime test

When the ELF is launched on Flycast or a Dreamcast, the expected result is a continuous 440 Hz stereo tone. This checks the KallistiOS audio stream startup, polling, callback delivery and AICA output path independently of PS1 game playback.

## Limitations

The smoke application has not yet been launched and auditioned in Flycast or on physical Dreamcast hardware. Successful cross-compilation and ELF creation do not establish CDI packaging, Flycast playback, real-time performance, or game compatibility. It does not yet initialize the PS1 machine or feed the SPU mixer into AICA; it deliberately emits a diagnostic tone only.
