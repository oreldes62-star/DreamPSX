# ISO-9660 directory reader foundation — milestone 4.14

## Delivered

- Added a bounded, read-only ISO-9660 reader for caller-owned in-memory images.
- Mount validates the Primary Volume Descriptor at logical sector 16, the CD001 identifier/version, volume-space size in both endian encodings, and the root directory record.
- Path lookup supports absolute paths, case-insensitive ASCII identifiers, optional ;version suffixes, and nested directories.
- File reads are bounded by the resolved file extent and the mounted image.
- Directory records with invalid lengths, inconsistent endian fields, or out-of-image extents are rejected.
- The API explicitly rejects directories as file-read targets and currently supports single-extent files only.

## Validation

The host CTest suite includes a synthetic ISO image with a root SYSTEM.CNF, a nested GAME directory, and a PS-X EXE-like payload. Tests cover mount validation, root lookup, nested lookup, case folding, ;version removal, bounded reads, missing paths, and malformed PVD rejection.

## Limitations

- No CUE/BIN/CDI file loader; images must be provided as a contiguous in-memory 2048-byte-sector ISO.
- No Joliet/Rock Ridge extensions, multi-extent files, interleaved files, or extended-attribute blocks.
- No boot configuration parser or PS-X EXE loader is connected yet.
- Host tests do not validate Dreamcast builds, CDI packaging, Flycast, GDEMU, or real games.
