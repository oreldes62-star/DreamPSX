# Dreamcast VFS-backed boot smoke application — milestone 4.63

## Objective

Provide a Dreamcast executable that exercises the complete file-backed CUE/BIN -> ISO9660 -> SYSTEM.CNF -> PS-X EXE path using KallistiOS VFS, rather than only testing the adapter in host code.

## Delivered

- Added `platform/dreamcast/audio-smoke/boot_smoke.c` as a separate boot diagnostic executable.
- Reads an exact 512 KiB PS1 BIOS file from `/cd/PSX/BIOS.BIN` into caller-owned storage.
- Opens `/cd/PSX/GAME.CUE` through the KallistiOS VFS CUE adapter. The CUE's quoted relative BINARY filename resolves in the same `/cd/PSX/` directory.
- Mounts the file-backed disc reader in ISO9660 and the emulated CD-ROM through `psx_machine_boot_from_disc_image()`.
- Resolves `SYSTEM.CNF`, loads the selected PS-X EXE and initializes the CPU entry, GP and SP.
- Connects the machine audio queue to the AICA stream and advances the PS1 system clock in approximately one-millisecond cycle batches.
- Keeps the existing 440 Hz audio-only smoke executable as a separate target.
- The KOS build now links and verifies both ELF targets and uploads them together.

## Test-disc file layout

The Dreamcast boot disc must expose these files through KOS's `/cd` filesystem:

```
/cd/PSX/BIOS.BIN
/cd/PSX/GAME.CUE
/cd/PSX/GAME.BIN
```

The CUE must reference the BIN using a relative quoted filename, for example `FILE "GAME.BIN" BINARY`, and contain one supported `TRACK 01` and `INDEX 01`. The BIOS must be exactly 524,288 bytes. BIOS and game files are supplied by the user; none are included in the repository or build artifact.

## Validation

- Host build and CTest passed on commit `75626fb9b1101e02661c1751ece7bb44ae4420a8`: workflow run 46080511885, 45/45 tests passed.
- Pinned KallistiOS/Dreamcast build passed on the same commit: workflow run 46080512000. Both the audio smoke ELF and new boot smoke ELF compiled; SH-4 ELF verification passed; artifact `dreampsx-dreamcast-build` ID 15237421252 (2,416,200 bytes) uploaded.

## Explicit limitations

- This is a startup/CPU/audio diagnostic, not yet a playable emulator frontend: GPU frame presentation and controller input are not wired into this app.
- The BIOS file is loaded into the PS1 memory model, but the current boot API directly loads the PS-X EXE and sets PC/GP/SP; it does not execute the full retail BIOS startup sequence.
- No commercial title has yet been run in Flycast or on physical Dreamcast hardware.
- The artifact contains ELF files and a core archive, not a CDI. The Dreamcast disc image layout and CDI packaging are a separate next milestone.
- Single-track CUE only; multi-track, CDDA and full XA scheduling remain incomplete.
