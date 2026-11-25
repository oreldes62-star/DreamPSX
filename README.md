# DreamPSX
PlayStation emulator research and development for Sega Dreamcast.

## Development status
Experimental project. The complete source snapshot is distributed as a ZIP; this GitHub repository tracks architecture notes and source modules and is not yet a complete buildable checkout.

## Current milestones
- [COP2 CU2 milestone 1.2](docs/COP2-CU2-MILESTONE-1.2.md)
- [COP2 BC2F/BC2T milestone 1.3](docs/COP2-BC2-MILESTONE-1.3.md)
- [COP2 LWC2/SWC2 milestone 1.4](docs/COP2-LWC2-SWC2-MILESTONE-1.4.md)
- [Memory milestone 1.5](docs/MEMORY-MILESTONE-1.5.md) — RAM mirrors, scratchpad and BIOS design/implementation snapshot.
- [IRQ milestone 1.6](docs/IRQ-MILESTONE-1.6.md) — standalone interrupt-controller core and test source.
- [IRQ MMIO adapter milestone 1.8](docs/IRQ-MMIO-MILESTONE-1.8.md) — byte/halfword/word accessors for I_STAT and I_MASK.
- [COP0 IRQ milestone 1.9](docs/COP0-IRQ-MILESTONE-1.9.md) — Cause.IP2 mirroring and Status/Cause interrupt eligibility helper.
- [IRQ/COP0 integration milestone 2.0](docs/IRQ-COP0-INTEGRATION-MILESTONE-2.0.md) — module-level bridge from IRQ controller output to COP0 Cause.IP2.
- [Machine state milestone 2.1](docs/MACHINE-STATE-MILESTONE-2.1.md) — shared machine container and interrupt coordination.
- [Memory backing-store milestone 2.2](docs/MEMORY-BACKING-STORE-MILESTONE-2.2.md) — 2 MiB RAM mirrors, KSEG0/KSEG1 aliases, scratchpad and read-only BIOS.
- [CPU bus core milestone 2.3](docs/BUS-CORE-MILESTONE-2.3.md) — unified CPU-facing routing for memory and first IRQ MMIO registers.
- [Machine/bus integration milestone 2.4](docs/MACHINE-BUS-INTEGRATION-MILESTONE-2.4.md) — machine-owned bus binding memory and IRQ into one CPU-facing interface.

## Target
Sega Dreamcast with 16 MiB RAM. Long-term target: CDI test build for Flycast, then GDEMU.

## Limitations
No Dreamcast/KallistiOS build, CDI image or playable game exists yet. The machine-owned bus currently routes RAM, scratchpad, BIOS, I_STAT and I_MASK. It is not yet connected to a complete CPU execution loop, and GPU, DMA, timers, CD-ROM, SPU, controllers and other MMIO devices remain to be implemented.
