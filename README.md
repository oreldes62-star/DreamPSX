# DreamPSX
PlayStation emulator research and development for Sega Dreamcast.

## Development status
Experimental project. The complete source snapshot is distributed as a ZIP; this GitHub repository tracks architecture notes and source modules and is not yet a complete buildable checkout.

## Current milestones
- [COP2 CU2 milestone 1.2](docs/COP2-CU2-MILESTONE-1.2.md)
- [COP2 BC2F/BC2T milestone 1.3](docs/COP2-BC2-MILESTONE-1.3.md)
- [COP2 LWC2/SWC2 milestone 1.4](docs/COP2-LWC2-SWC2-MILESTONE-1.4.md)
- [Memory milestone 1.5](docs/MEMORY-MILESTONE-1.5.md)
- [IRQ milestone 1.6](docs/IRQ-MILESTONE-1.6.md)
- [IRQ MMIO adapter milestone 1.8](docs/IRQ-MMIO-MILESTONE-1.8.md)
- [COP0 IRQ milestone 1.9](docs/COP0-IRQ-MILESTONE-1.9.md)
- [IRQ/COP0 integration milestone 2.0](docs/IRQ-COP0-INTEGRATION-MILESTONE-2.0.md)
- [Machine state milestone 2.1](docs/MACHINE-STATE-MILESTONE-2.1.md)
- [Memory backing store milestone 2.2](docs/MEMORY-BACKING-STORE-MILESTONE-2.2.md)
- [CPU bus core milestone 2.3](docs/BUS-CORE-MILESTONE-2.3.md)
- [Machine/bus integration milestone 2.4](docs/MACHINE-BUS-INTEGRATION-MILESTONE-2.4.md)
- [R3000A context and BIOS fetch milestone 2.5](docs/R3000A-CONTEXT-MILESTONE-2.5.md)
- [First R3000A execution milestone 2.6](docs/R3000A-EXECUTION-MILESTONE-2.6.md)
- [R3000A integer and memory expansion milestone 2.7](docs/R3000A-INTEGER-MEMORY-MILESTONE-2.7.md)
- [R3000A COP0 exception entry milestone 2.8](docs/R3000A-COP0-EXCEPTIONS-MILESTONE-2.8.md)
- [R3000A delay-slot tracking milestone 2.9](docs/R3000A-DELAY-SLOT-MILESTONE-2.9.md)
- [R3000A COP0 instruction support milestone 3.0](docs/R3000A-COP0-INSTRUCTIONS-MILESTONE-3.0.md)
- [Automatic hardware interrupt dispatch milestone 3.1](docs/AUTOMATIC-IRQ-DISPATCH-MILESTONE-3.1.md)

## Target
Sega Dreamcast with 16 MiB RAM. Long-term target: CDI test build for Flycast, then GDEMU.

## Limitations
No Dreamcast/KallistiOS build, CDI image or playable game exists yet. CPU registers and BIOS instruction fetch are now represented, but instruction decoding/execution remains partial, and remaining hardware devices are not implemented. The repository is not yet a complete buildable checkout.
