# DreamPSX memory milestone 1.5 — RAM, scratchpad and BIOS

## Implemented

- 2 MiB main RAM, mirrored across the 8 MiB physical range 0x00000000–0x007FFFFF.
- MIPS KUSEG/KSEG0/KSEG1 aliases are translated through the 29-bit physical address mask.
- 1 KiB scratchpad at physical 0x1F800000–0x1F8003FF.
- 512 KiB BIOS window at physical 0x1FC00000–0x1FC7FFFF.
- BIOS loading requires an exact 512 KiB image; writes to BIOS are ignored.
- Little-endian byte, halfword and word accessors and a PsxBus adapter.
- Host tests for RAM mirrors, cached/uncached aliases, scratchpad, BIOS read-only behavior and bus callbacks.

## Validation

`make clean && make test` passes for CPU, GTE and memory tests with the Makefile warning flags.

## Not implemented yet

- Memory-mapped hardware registers (GPU, DMA, timers, CD-ROM, SPU, controller and interrupt controller).
- Bus access faults, open-bus electrical behavior and timing/wait states.
- BIOS boot execution, Dreamcast/KallistiOS build and CDI generation.
- This is an initial RAM/BIOS/scratchpad backing store, not yet a complete PS1 memory bus.
