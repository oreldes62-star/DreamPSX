# Full DreamPSX core SuperH cross-compilation — milestone 4.47

## Goal

Move beyond compiling only the audio adapter: compile every current DreamPSX core translation unit with the real KallistiOS SH-4 cross-toolchain.

## Build changes

The Dreamcast audio smoke-test Makefile now builds `libdreampsx_core.a` from all 23 core/platform source files:
- bus, memory, IRQ, DMA, CD-ROM and SPU;
- SPU ADPCM, PCM queue and Dreamcast callback bridge;
- CPU, COP0 and GTE;
- GPU;
- ISO-9660, CUE/BIN, XA, boot configuration and PS-X EXE loader;
- machine scheduler and timers;
- KallistiOS AICA stream backend.

The 440 Hz diagnostic application is then linked as a separate ELF against that archive.

## Validation

The KallistiOS workflow completed successfully:
https://github.com/oreldes62-star/DreamPSX/actions/runs/46069249655

It built the KallistiOS SDK, compiled all 23 DreamPSX objects into the static archive, linked the audio smoke-test ELF, verified the archive contains exactly 23 members, and inspected the executable header:
- ELF32;
- little-endian;
- executable;
- Renesas / SuperH SH.

Build artifacts:
https://github.com/oreldes62-star/DreamPSX/actions/runs/46069249655/artifacts/15232755420

The host CTest suite remains green at 44/44:
https://github.com/oreldes62-star/DreamPSX/actions/runs/46069241574

## Important scope distinction

This proves all 23 translation units compile for the Dreamcast SH-4 target and that the audio diagnostic app links. Because the core is a static archive, the linker only pulls objects needed by the diagnostic app; this does not yet prove that a complete emulator executable is linked or boots a PS1 game.

No CDI packaging, Flycast runtime test, physical Dreamcast audio test, BIOS boot or commercial game compatibility test has been completed.

## Next step

Create a real Dreamcast runtime shell that initializes `PsxMachine`, binds its audio output queue to the AICA stream, and prepares safe media/BIOS loading without embedding copyrighted BIOS or game data.
