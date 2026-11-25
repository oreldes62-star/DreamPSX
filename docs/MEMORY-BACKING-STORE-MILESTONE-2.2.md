# DreamPSX milestone 2.2 — core memory backing store

## Added

- `include/dreampsx/memory.h`
- `src/bus/memory.c`
- `tests/test_memory.c`

## Implemented map

- 2 MiB caller-owned main RAM mirrored across physical 0x00000000–0x007FFFFF.
- KSEG0 (0x80000000–0x9FFFFFFF) and KSEG1 (0xA0000000–0xBFFFFFFF) direct aliases through the 29-bit physical mask.
- 1 KiB scratchpad at 0x1F800000.
- 512 KiB caller-owned, read-only BIOS at 0x1FC00000.
- Little-endian 8/16/32-bit reads and writes with alignment checks.
- Unmapped accesses return false; BIOS writes are ignored.
- No dynamic allocation: caller supplies RAM and BIOS storage, keeping allocation policy under Dreamcast platform control.

## Validation scope

The test source covers RAM mirrors, KSEG aliases, scratchpad, BIOS read-only behavior,
little-endian accesses, alignment and unmapped addresses. The test is committed but
has not been compiled or run in the repository because this GitHub checkout still
does not contain a complete build system/source snapshot.

## Not yet included

This is backing storage only. It does not implement hardware MMIO, TLB translation
for KUSEG, bus wait states, open-bus behavior, DMA, timers, GPU, CD-ROM or SPU.
CPU virtual-address translation and exception behavior remain separate tasks.
