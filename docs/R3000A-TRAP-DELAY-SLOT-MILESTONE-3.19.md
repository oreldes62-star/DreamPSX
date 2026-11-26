# R3000A conditional traps in branch delay slots — milestone 3.19

## Scope

Added machine-level regression coverage for conditional trap instructions executed in a branch delay slot.

- A true TEQ in the delay slot raises the Trap exception (COP0 exception code 13).
- The exception records the branch instruction address in EPC and sets Cause.BD.
- A false TEQ in the delay slot completes normally, follows the branch target, and does not raise a new trap.
- The false-condition test explicitly resets Cause before execution so it cannot inherit the previous test's exception code.

## Validation

GitHub Actions host CI passed on commit `dca77909d3df0d66ae0d49de1d9f5890a44f3e3c`; all 12 CTest targets passed.

This validates the host-side CPU/machine regression suite only. It is not a Dreamcast/KallistiOS build, BIOS boot, CDI image, or gameplay test.
