# Machine-level ISO game boot and first instruction — milestone 4.17

## Delivered

- Added `psx_machine_boot_from_iso` as the machine-level boot entry point.
- The machine resolves SYSTEM.CNF, loads its selected PS-X EXE into main RAM, and resets the CPU execution context.
- Initializes the R3000A program counter from the EXE entry point, global pointer register ($gp / r28), and stack pointer ($sp / r29).
- Clears pending delay-slot and load-delay state before execution.
- Added a synthetic ISO integration test that boots a PS-X EXE and executes its first MIPS instruction (`addiu v0, zero, 42`) through `psx_machine_step`.

## Validation

GitHub Actions host build and CTest passed: 38/38 tests.

This validates the host implementation and the synthetic boot path. It does not validate real commercial discs, the full BIOS startup environment, Dreamcast SH-4 performance, CDI packaging, Flycast, or GDEMU.

## Remaining boot limitations

- The boot path currently loads one PS-X EXE from an already-mounted ISO-9660 image. CUE/BIN/CDI mounting and image-file parsing are not implemented.
- BIOS A0/B0 runtime services, memory-card initialization, controller setup, CD-ROM startup handshakes, and full hardware startup behavior are not emulated as a complete boot environment.
- BOOT/BOOT2 fallback policy is basic; the first recognized entry is selected.
- CPU execution coverage and timing are still incomplete, so passing the synthetic first-instruction test is not equivalent to game compatibility.
