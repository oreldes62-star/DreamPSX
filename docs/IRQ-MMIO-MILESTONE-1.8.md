# DreamPSX milestone 1.8 — IRQ MMIO adapter

## Added

- `include/dreampsx/irq_mmio.h`
- `src/bus/irq_mmio.c`
- `tests/test_irq_mmio.c`

This adapter provides little-endian byte, halfword and word accessors for I_STAT and I_MASK. It preserves the untouched byte lane for byte writes, applies I_STAT write-zero-to-clear semantics through the existing controller, ignores the upper half of a word write, and rejects misaligned halfword/word accesses and unmapped addresses.

## Scope

This is a bus-facing adapter around the standalone IRQ controller. It is not yet connected to the main CPU memory bus, CPU COP0 Cause register, or a full PS1 machine object. The main repository still lacks the complete source tree and build system, so these files have not been compiled or run in the project build.

## Integration requirements

The eventual bus implementation must route accesses in the I/O range to these functions and translate a false return into the CPU's appropriate address/bus exception. CPU interrupt polling must separately mirror `psx_irq_output()` into COP0 Cause.IP2; this adapter must not take CPU exceptions or acknowledge requests on its own.
