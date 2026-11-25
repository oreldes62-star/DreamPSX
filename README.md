# DreamPSX
PlayStation emulator research and development for Sega Dreamcast.

## Development status
Experimental project. This GitHub repository tracks architecture notes, implementation notes, and source patches. The complete source snapshot is distributed separately as a ZIP; this repository is not yet a complete, buildable emulator checkout.

## Current milestones
- [PS1 memory bus and interrupt design](docs/PSX-MEMORY-BUS-DESIGN.md) — architecture specification only.
- [COP2 CU2 milestone 1.2](docs/COP2-CU2-MILESTONE-1.2.md) — CU2 gate and Coprocessor Unusable exception behavior.
- [COP2 BC2F/BC2T milestone 1.3](docs/COP2-BC2-MILESTONE-1.3.md) — branch behavior based on the PSX-SPX documented observation.
- [BC2F/BC2T source patch](patches/cop2-bc2f-bc2t-1.3.patch).
- [COP2 LWC2/SWC2 milestone 1.4](docs/COP2-LWC2-SWC2-MILESTONE-1.4.md) — aligned GTE data-register memory transfers, CU2 and address-error handling.
- [COP2 source patch, cumulative from source 1.2](patches/cop2-1.2-to-1.4.patch).

## Target
Sega Dreamcast with 16 MiB RAM. The long-term plan is a CDI test build for Flycast, followed by GDEMU support.

## Limitations
No Dreamcast/KallistiOS build, CDI image, or playable game is available yet. CPU and GTE work remains experimental and requires further validation.
