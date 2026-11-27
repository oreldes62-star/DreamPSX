# Raw CUE/BIN machine boot — milestone 4.21

## Delivered

- Added `psx_cdrom_mount_disc` so a parsed disc descriptor can be shared by the CD-ROM and filesystem layers without parsing the CUE twice.
- Added `psx_machine_boot_from_cue`.
- The machine parses the supplied single-track CUE/BIN description, mounts ISO-9660 through the sector-backed reader, mounts the same track in the CD-ROM controller, resolves `SYSTEM.CNF`, loads the selected PS-X EXE and initializes the CPU PC, GP and SP.
- The raw BIN stays caller-owned and is not expanded into a second 2048-byte-sector image.
- The CD-ROM controller continues to use the mounted track for guest ReadN requests after boot.

## Validation

GitHub Actions host build and CTest passed: 39/39 tests.

The machine boot integration test creates a synthetic ISO filesystem, wraps its sectors as raw MODE1/2352 sectors, mounts it through a CUE description, resolves a nested executable through `SYSTEM.CNF`, checks the CPU entry/GP/SP handoff, executes the first MIPS instruction, and reads sector zero through the mounted CD-ROM controller.

Validation run: https://github.com/oreldes62-star/DreamPSX/actions/runs/46059925579

## Ownership and memory

The CUE text is parsed during the boot call. The BIN byte buffer remains caller-owned and must remain valid for the lifetime of the mounted CD-ROM. The CD-ROM and ISO objects retain compact track descriptors and use the existing 2048-byte sector staging buffer; they do not allocate a full expanded ISO image.

## Current limitations

- The CUE parser remains limited to one BINARY file and one data track, with MODE1/2048, MODE1/2352 or MODE2/2352 Form 1.
- Multi-track audio/data discs, multiple BIN files, CDI containers, WAV, XA/CDDA, full pregap/index semantics and robust EDC/ECC validation remain unsupported.
- The current boot path loads a PS-X EXE directly and initializes the CPU. It does not reproduce the complete retail BIOS startup sequence, system library initialization, memory-card/controller setup or copy-protection behavior.
- Host CI does not validate SH-4 compilation, CDI packaging, Flycast, GDEMU or physical Dreamcast hardware.
