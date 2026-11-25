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

## Target
Sega Dreamcast with 16 MiB RAM. Long-term target: CDI test build for Flycast, then GDEMU.

## Limitations
No Dreamcast/KallistiOS build, CDI image or playable game exists yet. The IRQ module is not yet integrated into the CPU or memory-mapped I/O bus.
