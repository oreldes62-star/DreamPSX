# R3000A REGIMM branch instructions — milestone 3.9

## Implemented

The CPU decoder now supports the four classic R3000A REGIMM branch forms:

- `BLTZ` — branch when the signed source register is negative.
- `BGEZ` — branch when the signed source register is non-negative.
- `BLTZAL` — branch when negative and write `PC + 8` to `$ra` regardless of whether the branch is taken.
- `BGEZAL` — branch when non-negative and write `PC + 8` to `$ra` regardless of whether the branch is taken.

The branch delay slot is tracked for taken and not-taken REGIMM branches. A REGIMM control transfer attempted from a delay slot is classified as unpredictable, consistently with the existing control-transfer guard. Unsupported REGIMM sub-op encodings return Reserved Instruction.

## Regression coverage added

The CPU execution test source checks taken and not-taken branches, both link variants on taken and not-taken conditions, delay-slot behavior, and rejection of an unsupported REGIMM sub-op. A follow-up audit corrected the link behavior: the R3000A writes `$ra` for BLTZAL/BGEZAL even when the condition is false.

## Validation status

The implementation and regression source have been committed. They have **not yet been compiled or executed** in this editing session. The automated host CI must confirm the build and test results before this milestone is considered validated.

## Next step

Run the host CI, inspect any compiler or assertion failures, and then expand the R3000A integer/branch instruction coverage based on the results.
