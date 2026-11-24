# DreamPSX — PS1 memory bus and interrupt design

Status: architecture specification, not yet an implementation.

## Goals
- Centralize CPU-visible address decoding instead of embedding device checks in the CPU.
- Normalize cached/uncached MIPS segments to physical addresses before decoding.
- Keep PS1 main RAM, scratchpad, BIOS, GPU VRAM and SPU RAM separate.
- Make interrupt requests deterministic and independently testable.
- Avoid allocating duplicate RAM for address aliases.

## CPU address map

| Physical range | Size | Device | Initial behavior |
|---|---:|---|---|
| 0x00000000–0x001FFFFF | 2 MiB | Main RAM | Read/write; mirror aliases decoded by physical mask |
| 0x1F000000–0x1F7FFFFF | up to 8 MiB | Expansion region | Open bus until an expansion device is attached |
| 0x1F800000–0x1F8003FF | 1 KiB | Scratchpad | Read/write |
| 0x1F801000–0x1F801FFF | 4 KiB | I/O registers | Dispatch to device register handlers |
| 0x1F802000–0x1F802FFF | expansion I/O | DEV8 | Open bus until implemented |
| 0x1FA00000–0x1FBFFFFF | 2 MiB | DEV1 | Open bus until implemented |
| 0x1FC00000–0x1FC7FFFF | 512 KiB | BIOS ROM | Read-only |

The 2 MiB RAM, 1 KiB scratchpad and 512 KiB BIOS must be distinct backing stores. GPU VRAM (1 MiB) and SPU RAM (512 KiB) are device-owned memories, not CPU main-RAM aliases.

## Proposed module boundary

PsxBus owns or references:
- 2 MiB main RAM
- 1 KiB scratchpad
- 512 KiB BIOS image (read-only after loading)
- I/O device callbacks/register blocks
- interrupt controller state

Suggested API (names are provisional until reconciled with the existing CPU headers):
- psx_bus_init(bus, bios_data, bios_size)
- psx_bus_read8/read16/read32(bus, address, out_value)
- psx_bus_write8/write16/write32(bus, address, value)
- psx_bus_request_irq(bus, irq_bit)
- psx_bus_clear_irq(bus, irq_bit)

The CPU should depend on this interface rather than knowing individual peripheral register implementations. Alignment faults and load/store exception policy remain the CPU's responsibility; the bus reports unmapped/read-only accesses according to a documented result code.

## Interrupt controller

Registers:
- I_STAT at physical 0x1F801070 (16-bit status/acknowledge).
- I_MASK at physical 0x1F801074 (16-bit mask).

Interrupt sources:
- bit 0 VBlank
- bit 1 GPU
- bit 2 CD-ROM
- bit 3 DMA
- bits 4–6 timers 0–2
- bit 7 controller/memory card
- bit 8 SIO
- bit 9 SPU
- bit 10 lightpen / PIO / DTL shared source

Semantics:
- A device raises its source by requesting the corresponding I_STAT bit.
- Writing zero to an I_STAT bit acknowledges/clears that pending bit; writing one leaves it unchanged.
- I_MASK bits set to one enable the corresponding source.
- The CPU external interrupt request is asserted when (I_STAT & I_MASK) != 0.
- The PS1 presents this external request through COP0 Cause bit 10. CPU exception entry must still obey COP0 Status interrupt-enable/mask rules.
- Device-level acknowledgement may also be required; clearing I_STAT alone must not silently reset a device's internal condition.

## Implementation sequence
1. Add bus types and address normalization helpers.
2. Add RAM and scratchpad read/write paths, including width and boundary tests.
3. Add BIOS loading and read-only behavior.
4. Add I_STAT/I_MASK register semantics and IRQ-to-COP0 line propagation.
5. Connect CPU loads/stores and exception handling to the bus.
6. Add device callbacks for DMA, GPU, CD-ROM, timers, SIO and SPU incrementally.

## Required tests
- KUSEG/KSEG0/KSEG1 aliases reach the same physical RAM byte.
- RAM mirror behavior is consistent at region boundaries.
- Scratchpad boundaries and all access widths.
- BIOS reads succeed; BIOS writes do not alter ROM contents.
- I_STAT write-zero-to-clear semantics preserve unrelated pending bits.
- I_MASK gates the CPU IRQ line.
- Multiple simultaneous IRQ sources remain pending independently.
- COP0 does not take an interrupt when global IE or the relevant mask is disabled.

## References
- PSX-SPX memory map: https://psx-spx.consoledev.net/memorymap/
- PSX-SPX I/O map: https://psx-spx.consoledev.net/dev/ps1/system/iomap/
- PSX-SPX interrupts: https://psx-spx.consoledev.net/interrupts/
- KallistiOS Dreamcast SDK: https://github.com/KallistiOS/KallistiOS

This document records the intended design only. It does not claim that the bus or interrupt controller has been implemented or tested.