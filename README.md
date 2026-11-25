# DreamPSX
PlayStation emulator research and development for Sega Dreamcast.

## Development status
Experimental project. The complete source snapshot is distributed as a ZIP; this GitHub repository tracks architecture notes and source patches and is not yet a complete buildable checkout.

## Current milestones
- [COP2 CU2 milestone 1.2](docs/COP2-CU2-MILESTONE-1.2.md)
- [COP2 BC2F/BC2T milestone 1.3](docs/COP2-BC2-MILESTONE-1.3.md)
- [COP2 LWC2/SWC2 milestone 1.4](docs/COP2-LWC2-SWC2-MILESTONE-1.4.md)
- [Memory milestone 1.5](docs/MEMORY-MILESTONE-1.5.md) — RAM mirrors, scratchpad, BIOS and PsxBus adapter.

## Target
Sega Dreamcast with 16 MiB RAM. Long-term target: CDI test build for Flycast, then GDEMU.

## Limitations
No Dreamcast/KallistiOS build, CDI image or playable game exists yet. The current memory module does not implement memory-mapped hardware registers or bus timing.
