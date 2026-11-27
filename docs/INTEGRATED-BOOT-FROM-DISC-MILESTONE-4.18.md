# Integrated boot-from-disc and CD-ROM mounting — milestone 4.18

## Delivered

- Added `psx_machine_boot_from_disc`, accepting a caller-owned ISO-9660 image with 2048-byte sectors.
- Validates the ISO volume descriptor and directory tree before mounting the same image into the machine's CD-ROM controller.
- Loads the executable selected by SYSTEM.CNF, initializes CPU PC/GP/SP through the existing machine boot path, and leaves the disc mounted for guest CD-ROM reads.
- If executable loading fails after mounting, the CD-ROM media is unmounted and IRQ2 is refreshed.
- Extended the synthetic machine boot test to verify the mounted image remains available and that ReadN can retrieve sector-zero data after game startup.

## Validation

GitHub Actions host build and CTest passed: 38/38 tests. The new `test_machine_boot` verifies ISO boot, first MIPS instruction execution, mounted media identity, and a CD-ROM sector data read.

## Ownership and limitations

- The image buffer remains caller-owned and must remain valid for the entire time the CD-ROM is mounted. The emulator does not copy or free the disc image.
- This API accepts an already-decoded 2048-byte-sector ISO image. CUE/BIN raw-sector parsing, CDI container parsing, file-backed media streaming, 2352-byte sectors, subchannel data, XA audio, and CDDA are not implemented.
- BIOS startup services and full commercial-game compatibility remain incomplete.
- Host CI does not validate Dreamcast SH-4 compilation, CDI packaging, Flycast, GDEMU, or real hardware.
