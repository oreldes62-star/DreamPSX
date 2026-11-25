# DreamPSX milestone 2.6 — first R3000A instruction execution

## Delivered

- Added `psx_cpu_step`, which fetches one instruction through the machine bus, decodes a supported subset and commits register/PC state.
- SPECIAL subset: SLL, SRL, SRA, JR, ADDU, SUBU, AND, OR, XOR, NOR, SLT and SLTU.
- Immediate/control subset: J, JAL, BEQ, BNE, BLEZ, BGTZ, ADDIU, ANDI, ORI, XORI and LUI.
- Memory subset: LW and SW through the unified bus.
- Implemented delayed-branch sequencing: the instruction after a taken branch/jump executes before control reaches the target. JAL writes the return address to r31.
- Added a test program for arithmetic, branch delay-slot behavior and bus-backed load/store.

## Deliberate limitations

This is an initial interpreter slice, not full R3000A compatibility. ADDI overflow exceptions, loads/stores other than LW/SW, unaligned access emulation, COP0 instructions, exceptions/interrupt entry, branch-likely opcodes, multiply/divide, cache behavior and cycle timing are not implemented. Unsupported instructions return false without advancing PC. A bus failure also returns false.

## Validation

The test source is committed but has not been executed in a complete project build. The repository still lacks a complete build system/source checkout, and no Dreamcast build or CDI exists.

## Next

Expand the integer instruction set and add precise exception handling, then connect COP0 state and interrupt checks to instruction boundaries.
