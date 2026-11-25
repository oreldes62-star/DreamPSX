# DreamPSX milestone 1.6 — interrupt-controller core

## Added module

- `include/dreampsx/irq.h`
- `src/bus/irq.c`
- `tests/test_irq.c`

The standalone controller models the 11 defined PS1 interrupt sources, source input levels, edge-latched pending bits, I_STAT write-zero-to-clear behavior, I_MASK, and the masked IRQ output.

## Semantics

- A request is latched only on a source low-to-high transition.
- Keeping a source high does not create repeated requests.
- Clearing I_STAT while the source remains high does not immediately relatch it; the source must go low and rise again.
- I_MASK gates the CPU-facing output without deleting pending requests.
- Bits 11–15 are ignored.

## Integration status

This is a standalone module and test, not yet wired into the CPU, memory bus, or device register map. The caller must reflect `psx_irq_output()` into COP0 Cause.IP2 and apply COP0 Status interrupt-enable/mask rules in CPU exception processing.

## Validation status

The test source covers edge capture, held-high behavior, write-zero-to-clear, mask gating, independent sources, and unused bits. It has not been run in the full project build in this milestone because the GitHub repository does not yet contain the complete source tree/build setup.
