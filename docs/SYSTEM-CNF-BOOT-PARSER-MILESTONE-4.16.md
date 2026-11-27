# SYSTEM.CNF boot configuration parser — milestone 4.16

## Delivered

- Parses BOOT and BOOT2 assignments from a bounded SYSTEM.CNF text buffer.
- Accepts case-insensitive CDROM device prefixes and whitespace around assignments.
- Converts CD-ROM path separators to ISO-style slash separators.
- Resolves SYSTEM.CNF from a mounted ISO, reads its bounded contents, parses the selected boot path, and resolves the executable using the ISO-9660 directory reader.

## Validation

Dedicated host tests cover BOOT, BOOT2, mixed-case device prefix, whitespace, CRLF, path normalization, unsupported device prefixes, and truncated paths.

## Limitations

- BOOT and BOOT2 are recognized, but fallback selection policy when both exist is not implemented; the first recognized assignment wins.
- No multi-disc handling, subchannel metadata, or copy-protection support.
- Executable loading and CPU register handoff remain separate operations; the machine does not yet boot a game end-to-end.
- Host CI does not validate Dreamcast builds, CDI packaging, Flycast, GDEMU, or real games.
