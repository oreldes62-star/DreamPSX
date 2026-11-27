# File-backed ISO9660 and machine boot integration — milestone 4.62

## Objective

Connect the KallistiOS VFS-backed sector reader to the existing ISO9660 and machine boot pipeline without expanding a disc image into a full in-memory buffer.

## Delivered

- Fixed `psx_iso9660_mount_disc()` so it accepts either an in-memory disc source or a valid random-access `read_at` callback. Previously, it incorrectly required `disc->source`, rejecting the new KOS file-backed adapter.
- Added `psx_machine_boot_from_disc_image()`, which mounts a caller-owned `PsxDiscImage` in both ISO9660 and the CD-ROM controller, resolves `SYSTEM.CNF`, loads the selected PS-X EXE, and initializes CPU PC/GP/SP.
- Refactored the existing in-memory CUE boot API to use the same common disc-image boot path.
- Added an end-to-end host test using a MODE1/2352 source exposed only through a random-access callback. It validates ISO9660 traversal, SYSTEM.CNF resolution, PS-X EXE loading, CPU entry/GP/SP setup, execution of the first MIPS instruction, and sector reads through the mounted CD-ROM reader.

## Ownership

The caller owns the `PsxDiscImage` reader context and any backing file handle. It must remain valid while the machine's CD-ROM is mounted. On Dreamcast, the `PsxKosDiscFile` object and its VFS handle must therefore outlive the mounted machine disc; close/unmount the CD-ROM before closing the file.

## Validation

- Host build and CTest passed on commit `545470d6f86a57b269621b660d81df142dfd36ed`: workflow run 46080120480, 45/45 tests passed. The existing `test_machine_boot` now also exercises the file-backed callback boot path.
- The pinned KallistiOS/Dreamcast build passed: workflow run 46080120557. The core archive and AICA smoke-test ELF compiled, SH-4 ELF verification passed, and artifact `dreampsx-dreamcast-build` ID 15236616724 (1,258,829 bytes) was uploaded.

The host test exercises the same callback-based path used by the KallistiOS file adapter. This does not yet prove that a retail PS1 title boots in Flycast or on hardware. BIOS startup sequencing, game-specific compatibility, CDI packaging and Flycast runtime remain separate validation steps.
