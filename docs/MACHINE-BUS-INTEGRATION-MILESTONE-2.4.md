# DreamPSX milestone 2.4 — machine/bus integration

## Delivered

- Added a PsxBus member to PsxMachine.
- Machine initialization now creates the RAM/BIOS backing store, resets IRQ/COP0 state, then binds the bus to the machine-owned memory and IRQ controller.
- The CPU-facing bus is now reachable as machine.bus; it routes RAM, scratchpad, BIOS, I_STAT and I_MASK.
- Added an integration test that exercises RAM through KSEG0/KSEG1 aliases and IRQ MMIO through physical and KSEG1 addresses.
- Verified in the test source that I_STAT write-zero-to-clear semantics survive the full machine-to-bus path.

## Ownership

PsxMachine owns its memory and IRQ controller. Its bus stores pointers to those members. Do not copy or relocate an initialized PsxMachine: doing so would leave the bus pointers referring to the original object's members. Initialize the machine in its final storage location.

Calling psx_machine_reset() resets IRQ and COP0 state without clearing RAM or scratchpad; the bus bindings remain valid.

## Validation status

 tests/test_machine_bus.c is committed as an integration test source. It has not been executed in a complete project build because this GitHub repository is still a collection of modules and documentation rather than a complete buildable checkout. No Dreamcast binary or CDI has been produced.

## Next

The next CPU milestone should introduce a stable R3000A register/context structure and connect instruction fetch and data accesses to machine.bus. The remaining hardware devices (GPU, DMA, timers, CD-ROM, SPU and controllers) still need bus adapters.
