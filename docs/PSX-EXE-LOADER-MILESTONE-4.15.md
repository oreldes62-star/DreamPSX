# PS-X EXE loader foundation — milestone 4.15

## Delivered

- Added a standalone PS-X EXE loader for the documented 0x800-byte header format.
- Parses entry PC, GP, load address, payload size, BSS range, and stack base/offset.
- Supports physical RAM addresses and KSEG0/KSEG1 aliases while enforcing the actual 2 MiB RAM boundary.
- Validates the complete payload and BSS ranges before modifying memory.
- Copies the executable payload and zeroes the BSS region.
- Returns parsed launch metadata to the caller; it does not yet alter CPU registers or start execution.

## Validation

A synthetic executable test checks header decoding, KSEG0 load, payload bytes, BSS clearing, initial stack calculation, invalid magic, truncated image, unaligned entry PC, and payload/BSS out-of-range rejection without partial memory writes.

## Limitations

- The loader is not yet connected to ISO-9660 file lookup or SYSTEM.CNF parsing.
- No CPU entry/GP/SP handoff is performed.
- It assumes a contiguous in-memory PS-X EXE file and does not implement compressed or proprietary executable variants.
- Host tests do not validate Dreamcast builds, CDI packaging, Flycast, GDEMU, or real games.
