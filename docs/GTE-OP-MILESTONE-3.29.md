# GTE OP milestone 3.29

## Implemented

The GTE command dispatcher now supports OP (command 0x0C), the outer/cross product operation:

- MAC1 = IR3 × RT22 − IR2 × RT33.
- MAC2 = IR1 × RT33 − IR3 × RT11.
- MAC3 = IR2 × RT11 − IR1 × RT22.
- sf selects the 12-bit fixed-point shift.
- lm selects signed or non-negative IR saturation.
- MAC1–MAC3 and IR1–IR3 are updated, with per-channel overflow and saturation flags.

## Regression coverage

Tests use an identity diagonal rotation matrix and IR=(1,2,3) to verify the expected cross product in both sf=1 and sf=0 modes.

## Validation

GitHub Actions host build and CTest passed on commit `bb81107f865f693606cf387c2ab13c17c030e062`: all 13 test targets passed, including `test_gte` with both OP shift-mode cases.

This is host-side validation only; it does not establish Dreamcast/KallistiOS buildability, PS1 hardware accuracy, BIOS boot, CDI generation or game compatibility.

## Next work

Continue expanding the GTE command set, then add independent reference vectors for arithmetic edge cases and FIFO interactions.
