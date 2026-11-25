# R3000A integer and memory expansion — milestone 2.7

## Implemented

The interpreter now adds:
- signed/unsigned multiplication and division (MULT, MULTU, DIV, DIVU);
- HI/LO transfers (MFHI, MTHI, MFLO, MTLO);
- JALR;
- SLTI and SLTIU;
- byte and halfword loads/stores (LB, LBU, LH, LHU, SB, SH);
- portable arithmetic right shift and signed comparisons.

Division by zero is handled without invoking C undefined behavior. Signed division also handles the INT32_MIN / -1 corner case explicitly. ADDI detects signed overflow and stops execution rather than silently producing a wrapped result.

## Validation status

A focused test source was added at `tests/test_cpu_integer.c`. It covers signed multiplication, HI/LO, signed division, byte/halfword sign extension and division by zero.

**The test has been added but has not been compiled or executed.** The GitHub repository is still not a complete buildable checkout, and no Dreamcast/KallistiOS build or CDI has been produced.

## Remaining work

- Model CPU exceptions precisely (including ADDI overflow) instead of stopping the step.
- Add unaligned load/store instructions and their merge semantics.
- Implement remaining R3000A instructions, COP0 exception entry, interrupt dispatch and cycle timing.
- Integrate the full source tree and establish a reproducible Dreamcast build.
