# R3000A REGIMM branch-delay exception coverage — milestone 3.12

## Changes

- Added a machine-level regression with a normal branch followed by BLTZ in its delay slot.
- The CPU classifies the nested control transfer as unpredictable; the machine layer maps it to the reserved-instruction exception.
- The test verifies that EPC points to the original branch instruction and Cause.BD is set.

## Validation status

The regression is committed in `tests/test_machine.c`. The immediately preceding host revision passed all 12 CTest tests; the workflow for this new regression is running separately and must complete before this milestone can be marked fully validated.

## Next step

Continue checking R3000A control-flow and delayed-load interactions, then move toward a bootable BIOS execution path.
