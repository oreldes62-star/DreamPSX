# CUE/BIN CD-ROM integration — milestone 4.20

## Delivered

- Added `psx_cdrom_mount_cue` to mount a parsed single-track CUE/BIN layout directly in the CD-ROM controller.
- The controller stores a small `PsxDiscImage` descriptor and a non-owning pointer to the caller's BIN buffer; the image is not copied into a second 2048-byte-sector allocation.
- ReadN now maps the requested MSF address to track-relative LBA and asks the disc-image reader for one 2048-byte user-data sector.
- Existing ISO mounting remains supported and switches the controller back to its direct 2048-byte-sector path.
- Eject clears both media descriptors; reset preserves mounted media as before.
- The resulting 2048-byte sector enters the existing data FIFO, so byte reads and CD-ROM DMA can continue using the same downstream path.

## Validation

GitHub Actions host build and CTest passed: 39/39 tests. The CD-ROM test mounts a synthetic MODE1/2352 CUE/BIN track, issues Setloc and ReadN, enables data requests, reads the first word through the CD-ROM data FIFO, and checks that eject clears the mounted track. Existing ISO ReadN tests remain passing.

Validation run: https://github.com/oreldes62-star/DreamPSX/actions/runs/46059424007

## Memory and ownership

- The BIN image remains caller-owned and must remain valid while mounted.
- The controller holds only a descriptor and its existing 2048-byte sector buffer; it does not allocate or duplicate the full disc.
- The CUE parser currently accepts one BINARY file and one data track, MODE1/2048, MODE1/2352 or MODE2/2352 Form 1.
- This is not yet a filesystem-backed Dreamcast loader. The frontend must supply the CUE text and BIN bytes.

## Remaining limitations

Multi-track and mixed audio/data discs, multi-file CUE, CDI containers, WAV audio, pregap/index-00 behavior, XA audio and CDDA are not implemented. The ISO-9660 boot path still expects a contiguous 2048-byte-sector image; booting directly from a raw CUE/BIN image requires a sector-reader adapter for ISO directory/executable loading. No Dreamcast build, CDI packaging, Flycast, GDEMU or physical hardware validation is claimed.
