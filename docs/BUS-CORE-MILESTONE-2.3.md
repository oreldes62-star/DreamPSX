# CPU bus core — milestone 2.3

## Delivered

- Added a CPU-facing `PsxBus` that routes byte, halfword and word reads/writes.
- Routes RAM, scratchpad and BIOS accesses to the memory backing store.
- Routes I_STAT (0x1F801070) and I_MASK (0x1F801074) to the IRQ MMIO adapter.
- Normalizes KSEG0/KSEG1 addresses before MMIO dispatch, so cached and uncached aliases reach the same device.
- Preserves the existing memory and IRQ adapters' alignment, little-endian and write-zero-to-clear behavior.
- Rejects accesses not handled by either mapped subsystem.

## Ownership and lifecycle

The bus holds non-owning pointers to `PsxMemory` and `PsxIrqController`. Both objects must outlive the bus. Initialize memory and IRQ first, then call `psx_bus_init`. Do not copy a live bus while its owners are being moved.

## Validation status

`tests/test_bus.c` covers RAM aliases, IRQ MMIO routing, KSEG1 MMIO access, I_STAT clearing, unmapped registers and alignment rejection. The repository still does not contain a complete build system/source checkout, so this test has been added but not executed as part of a full project build.

## Not yet delivered

This is a first bus slice, not a complete PlayStation bus. GPU, DMA, timers, CD-ROM, joypad, expansion ports and other MMIO devices still need explicit adapters. The CPU execution loop is not yet connected.
