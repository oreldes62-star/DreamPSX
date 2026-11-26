# R3000A REGIMM link-register aliasing — milestone 3.11

## Changes

- Added a BLTZAL regression where the source register is `$ra` and its original value is negative. The branch must be taken using that original value before `$ra` is replaced with `PC + 8`.
- Added a BGEZAL regression where the source register is `$ra` and its original value is negative. The branch must remain not taken even though the instruction writes the positive link address to `$ra`.
- These cases protect operand-read-before-write ordering for the R3000A link variants.

## Validation status

The regression cases have been added to `tests/test_cpu_execution.c` and committed. GitHub Actions successfully configured CMake, built the core and all test executables, and passed all 12 CTest tests on commit `392bc827334fc223550bfdc8bbf4c46ed30c7e13`. This validates the host-side test suite for that revision; it does not validate a Dreamcast/KallistiOS build.

## Next step

Obtain a real host build and CTest result, then continue the instruction-set audit with the same emphasis on architectural corner cases and exception interactions.
