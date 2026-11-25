# R3000A delay-slot tracking — milestone 2.9

## Implemented

- Added explicit CPU state for a pending branch delay slot and the address of its originating branch.
- Tracks delay slots for J, JAL, JR, JALR and all conditional branches, including branches that are not taken.
- Clears the pending marker after the delay-slot instruction executes.
- Rejects a control-transfer instruction encountered in a delay slot rather than silently applying an unreliable nested-branch result.
- Added execution-test coverage for a not-taken branch and its delay-slot bookkeeping.

## Why this matters

COP0 exception entry needs to distinguish an exception in an ordinary instruction from one in a branch delay slot. In the latter case, EPC must identify the branch instruction and Cause.BD must be set. The CPU now retains the two pieces of state required by the machine-level exception path.

## Validation status

Test source has been added but has not been compiled or run. The repository remains an incomplete source checkout without a reproducible project build. The CPU still returns a boolean for step success/failure; automatic classification and dispatch of faults into COP0 exceptions remain future work.
