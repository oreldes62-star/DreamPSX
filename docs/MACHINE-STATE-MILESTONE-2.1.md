# DreamPSX milestone 2.1 — shared machine state

## Added

- `include/dreampsx/machine.h`: shared state structure.
- `src/machine/machine.c`: reset, IRQ-to-COP0 refresh, interrupt eligibility.
- `tests/test_machine.c`: state lifecycle and IRQ/COP0 flow test.

## Current machine state

The structure now owns:
- the standalone PS1 IRQ controller;
- COP0 Status;
- COP0 Cause.

The refresh function mirrors the controller's masked output into Cause.IP2.
The eligibility function checks Status.IEc and matching Status.IM/Cause.IP
bits. The state is deliberately small and uses fixed-width integer fields.

## Scope and remaining integration

This is the first shared machine-state container, not yet the complete PS1.
The R3000A register file, RAM, BIOS mapping, timers, DMA, GPU, SPU and
unified memory bus still need to be added. The current repository does not
yet contain a complete application build or Dreamcast CDI target.

Exception entry, EPC/BD handling and instruction-boundary scheduling remain
CPU-core responsibilities and are not implemented by this milestone.
