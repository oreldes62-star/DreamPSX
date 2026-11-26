# DreamPSX
PlayStation emulator research and development for Sega Dreamcast.

## Development status
Experimental project. The host-side CMake build and all 12 CTest tests passed in GitHub Actions on commit `a0866bf540d09a25dd534caca548ae904b39d7a2`. This validates the current host test suite only; the Dreamcast/KallistiOS target remains a separate future integration step.

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
- [Synchronous CPU exception dispatch milestone 3.2](docs/SYNCHRONOUS-CPU-EXCEPTIONS-MILESTONE-3.2.md)
- [R3000A SYSCALL, BREAK and encoding validation milestone 3.3](docs/R3000A-SYSCALL-BREAK-VALIDATION-MILESTONE-3.3.md)
- [R3000A load delay slot milestone 3.4](docs/R3000A-LOAD-DELAY-MILESTONE-3.4.md)
- [R3000A LWL/LWR unaligned load milestone 3.5](docs/R3000A-LWL-LWR-MILESTONE-3.5.md)
- [R3000A memory access validation milestone 3.6](docs/R3000A-MEMORY-ACCESS-VALIDATION-MILESTONE-3.6.md)
- [R3000A delayed-load exception milestone 3.7](docs/R3000A-DELAYED-LOAD-EXCEPTION-MILESTONE-3.7.md)
- [Host build and automated test foundation milestone 3.8](docs/HOST-BUILD-MILESTONE-3.8.md)
- [R3000A REGIMM branch instructions milestone 3.9](docs/R3000A-REGIMM-BRANCHES-MILESTONE-3.9.md)
- [R3000A branch encoding validation milestone 3.10](docs/R3000A-BRANCH-ENCODING-MILESTONE-3.10.md)
- [R3000A REGIMM link-register aliasing milestone 3.11](docs/R3000A-REGIMM-LINK-ALIAS-MILESTONE-3.11.md)
- [R3000A REGIMM branch-delay exception milestone 3.12](docs/R3000A-REGIMM-DELAY-SLOT-MILESTONE-3.12.md)
- [R3000A delayed-load/control-flow milestone 3.13](docs/R3000A-DELAYED-LOAD-CONTROL-FLOW-MILESTONE-3.13.md)
- [R3000A delayed-load/link/exception milestone 3.14](docs/R3000A-DELAYED-LOAD-EXCEPTION-INTERACTIONS-MILESTONE-3.14.md)
- [R3000A SWL/SWR partial-word store milestone 3.15](docs/R3000A-SWL-SWR-MILESTONE-3.15.md)

## Target
Sega Dreamcast with 16 MiB RAM. Long-term target: CDI test build for Flycast, then GDEMU.

## Limitations
No Dreamcast/KallistiOS build, CDI image or playable game exists yet. CPU registers and BIOS instruction fetch are now represented, but instruction decoding/execution remains partial, and remaining hardware devices are not implemented. The host CMake/CTest suite passes in GitHub Actions, but it does not produce a Dreamcast binary or CDI image.
