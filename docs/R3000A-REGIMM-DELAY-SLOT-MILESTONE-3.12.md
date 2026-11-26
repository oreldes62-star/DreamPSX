# R3000A REGIMM branch-delay exception coverage — milestone 3.12

## Changes

- Added a machine-level regression with a normal branch followed by BLTZ in its delay slot.
- The CPU classifies the nested control transfer as unpredictable; the machine layer maps it to the reserved-instruction exception.
- The test verifies that EPC points to the original branch instruction and Cause.BD is set.

## Validation status

The regression is committed in `tests/test_machine.c`. GitHub Actions configured CMake, built the core and test executables, and passed all 12 CTest tests on commit `b6089eb2410f6d967dcea2c116bb295de5c69d39`. This confirms the host-side regression suite for that revision; it does not validate a Dreamcast/KallistiOS build.

## Next step

Continue checking R3000A control-flow and delayed-load interactions, then move toward a bootable BIOS execution path.
