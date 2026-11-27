# SYSTEM.CNF boot configuration and executable loading — milestone 4.16

## Delivered

- Parses BOOT and BOOT2 assignments from a bounded SYSTEM.CNF text buffer.
- Accepts case-insensitive CDROM device prefixes and whitespace around assignments.
- Normalizes backslash path separators to ISO-style slashes.
- Resolves SYSTEM.CNF from a mounted ISO and resolves its selected executable through the ISO-9660 directory reader.
- Reads the PS-X EXE file from the ISO into a bounded temporary buffer and passes it to the validated PS-X EXE loader.
- Returns boot path and executable metadata after a successful RAM load. The caller retains ownership of ISO image, RAM, and BIOS buffers.

## Validation

Dedicated host tests cover BOOT/BOOT2 parsing, CRLF, path normalization, unsupported device prefixes, empty paths, synthetic ISO directory traversal, SYSTEM.CNF resolution, PS-X EXE loading, and copied payload bytes.

## Limitations

- BOOT and BOOT2 are recognized, but fallback selection policy when both exist is not implemented; the first recognized assignment wins.
- CPU PC/GP/SP register handoff and actual machine execution are not yet connected.
- No multi-disc handling, subchannel metadata, or copy-protection support.
- Host CI does not validate Dreamcast builds, CDI packaging, Flycast, GDEMU, or real games.
