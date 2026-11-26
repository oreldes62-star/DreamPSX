# R3000A branch encoding validation — milestone 3.10

## Changes

- The decoder now validates the fixed `rt` field for `BLEZ` and `BGTZ`.
- A non-zero `rt` field on either instruction is classified as a Reserved Instruction.
- Regression cases were added for malformed BLEZ and BGTZ encodings.
- Temporary diagnostic output was removed from the machine exception test.

## Validation

The changes are committed to the repository. No GitHub Actions workflow runs or commit status checks were reported for these commits, and the test suite has not been compiled or executed in this session. This milestone is therefore source-updated but not yet build-validated.

## Next step

Set up or repair a host CI workflow so the full CMake test suite runs on every push. Then continue auditing branch and load-delay behavior against the R3000A specification.
