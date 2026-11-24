# DreamPSX — PS1 memory bus and interrupt design

Status: architecture specification, not yet an implementation.

## Goals
- Centralize CPU-visible address decoding instead of embedding device checks in the CPU.
- Normalize MIPS KUSEG/KSEG0/KSEG1 aliases to physical addresses where the hardware maps them.
- Keep PS1 main RAM, scratchpad, BIOS, GPU VRAM and SPU RAM separate.
- Model the actual RAM and peripheral mirrors without allocating duplicate backing stores.
- Make interrupt requests deterministic and independently testable.

## CPU address map

| Physical range | Size | Device | Initial behavior |
|---|---:|---|---|
| 0x00000000–0x001FFFFF | 2 MiB | Main RAM | Read/write; mirrored through the first 8 MiB by default |
| 0x1F000000–0x1F7FFFFF | 512 KiB default, up to 8 MiB | DEV0 expansion | Open bus until an expansion device is attached |
| 0x1F800000–0x1F8003FF | 1 KiB | Scratchpad | Read/write; special segment alias rules apply |
| 0x1F801000–0x1F801FFF | 4 KiB | I/O registers | Dispatch to device register handlers |
| 0x1F802000–0x1F802FFF | 8 KiB region | DEV8 expansion I/O | Open bus until implemented |
| 0x1FA00000–0x1FBFFFFF | 2 MiB | DEV1 expansion | Open bus until implemented |
| 0x1FC00000–0x1FC7FFFF | 512 KiB | BIOS ROM | Read-only; BIOS mirror into the last 4 MiB is disabled by default |

Additional CPU-visible aliases:
- KUSEG, KSEG0 and KSEG1 are address-segment aliases of the low physical address space; KSEG0 is cached and KSEG1 is uncached.
- The 2 MiB main RAM is mirrored in the first 8 MiB by default. Decode this with a single 2 MiB backing allocation; do not allocate copies for mirrors.
- The scratchpad is at physical 0x1F800000 and is mirrored in KUSEG and KSEG0, but not KSEG1. It is data fast RAM, not executable memory.
- BIOS mirroring across the final 4 MiB is configurable and disabled by default; do not assume every high address aliases the BIOS.
- The seven DMA control registers at 0x1F8010A8 are mirrored at 0x1F8010AC.

Memory not directly mapped to the CPU bus:
- GPU VRAM: 1 MiB, owned by the GPU.
- SPU sound RAM: 512 KiB, owned by the SPU.
- CD-ROM controller RAM/firmware/buffer and memory-card storage are device-owned.

The main RAM (2 MiB), scratchpad (1 KiB) and BIOS (512 KiB) must be distinct backing stores. GPU VRAM and SPU RAM must not be treated as main-RAM aliases.

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

The CPU should depend on this interface rather than knowing individual peripheral register implementations. Alignment faults and load/store exception policy remain the CPU's responsibility; the bus reports unmapped/read-only accesses according to a documented result code. Instruction fetch from scratchpad must be rejected as a bus error.

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
- bit 7 controller/memory card byte received
- bit 8 SIO
- bit 9 SPU
- bit 10 lightpen / PIO / DTL shared source
- bits 11–15 unused; upper 16 bits are not meaningful for this register.

Semantics:
- I_STAT request bits are edge-triggered: a source sets its pending bit on a false-to-true transition.
- Writing zero to an I_STAT bit acknowledges/clears that pending bit; writing one leaves it unchanged.
- I_MASK bits set to one enable the corresponding source.
- The external CPU interrupt request is asserted when (I_STAT & I_MASK) != 0 and is reflected in COP0 Cause bit 10.
- COP0 Cause bit 10 is a live reflection of the masked interrupt-controller output, not an independent software latch.
- CPU exception entry must still obey COP0 Status global interrupt enable and interrupt mask rules.
- Most peripheral sources also require acknowledgement in the peripheral itself. Correct order: acknowledge I_STAT first, then acknowledge the peripheral register, to avoid losing a new edge.
- VBlank and timer sources have different acknowledgement behavior from most peripheral sources; model each device's own condition separately.

## Implementation sequence
1. Add bus types and explicit segment/physical-address normalization helpers.
2. Add main RAM decode and its 8 MiB default mirror behavior.
3. Add scratchpad with its KUSEG/KSEG0-only aliases and non-executable fetch behavior.
4. Add BIOS loading, read-only behavior and configurable mirror policy.
5. Add I_STAT/I_MASK edge semantics and live IRQ-to-COP0 Cause bit 10 propagation.
6. Connect CPU loads/stores and exception handling to the bus.
7. Add device callbacks for DMA, GPU, CD-ROM, timers, SIO and SPU incrementally.

## Required tests
- KUSEG/KSEG0/KSEG1 RAM aliases resolve to the same physical backing byte.
- Main RAM mirrors across the first 8 MiB by default, including boundary addresses.
- Scratchpad works in KUSEG and KSEG0 aliases but does not alias through KSEG1.
- Instruction fetch from scratchpad reports a bus error.
- Scratchpad boundaries and all supported access widths.
- BIOS reads succeed; BIOS writes do not alter ROM contents.
- BIOS high mirror is absent by default and follows the configured memory-control setting when implemented.
- I_STAT write-zero-to-clear semantics preserve unrelated pending bits.
- A held-high source does not generate repeated edges without a low-to-high transition.
- Peripheral acknowledgement ordering permits a new edge to be captured.
- I_MASK gates the CPU IRQ line; Cause bit 10 follows the masked controller output.
- Multiple simultaneous IRQ sources remain pending independently.
- COP0 does not take an interrupt when global IE or the relevant mask is disabled.

## References
- PSX-SPX memory map: https://psx-spx.consoledev.net/memorymap/
- PSX-SPX I/O map: https://psx-spx.consoledev.net/iomap/
- PSX-SPX interrupts: https://psx-spx.consoledev.net/interrupts/
- KallistiOS Dreamcast SDK: https://github.com/KallistiOS/KallistiOS

This document records the intended design only. It does not claim that the bus or interrupt controller has been implemented or tested.