# CD-ROM ISO sector delivery — milestone 4.09

## Delivered

- Added a non-owning in-memory ISO mount API. The frontend retains ownership of the supplied image buffer.
- The accepted image format is a contiguous sequence of 2048-byte user-data sectors.
- Implemented BCD MSF validation and conversion to ISO LBA using the 150-sector pregap.
- Implemented the ReadN command for one sector at the current Setloc address.
- Added a 2048-byte sector buffer and byte-wise reads through the CD-ROM data register.
- Implemented the data-request gate (bank 2, register 2, bit 7), DRQ status reporting, and data-ready INT1 alongside command-complete INT3.
- Reset clears command/transfer state but leaves a mounted image in place; explicit unmount ejects it.
- Added tests for sector zero, first/last bytes, full 2048-byte drain, invalid image length, and media retention on reset.

## Scope and limitations

This milestone reads one 2048-byte data sector from a memory-resident ISO buffer. It is not yet a full disc subsystem: no filesystem/ISO parser, file-backed or CDI/CUE image loader, asynchronous continuous ReadN loop, seek/rotation latency, 2352-byte raw sectors, subchannel data, XA audio, CDDA, or real Dreamcast frontend integration. The host/frontend must provide and retain the image buffer.

## Validation

GitHub Actions run `46046829787`: host configure and build succeeded; CTest passed all 34/34 tests. This is host-side validation, not a Dreamcast build or game-compatibility result.
