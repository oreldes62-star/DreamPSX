# DreamPSX 1.2 — COP2 usability (CU2)

This milestone adds the first architectural gate for PlayStation COP2/GTE instructions.

## Implemented

- Status.CU2 is checked at the start of every COP2 instruction. It is bit 30 of COP0 Status.
- When CU2 is clear, the CPU raises Coprocessor Unusable (ExcCode 11).
- Cause.CE (bits 28–29) is set to 2 to identify COP2.
- The CPU records EPC and redirects execution to the general exception vector using the existing exception path.
- The check happens before the emulator's optional GTE pointer is tested.

## Tests

The CPU host test covers a COP2 instruction with CU2 disabled and asserts:
- exception type and ExcCode 11;
- Cause.CE = 2;
- EPC equals the faulting instruction address;
- PC is redirected to 0x80000080.

The existing COP2/GTE transfer and SQR tests now enable CU2 before executing COP2 instructions.

## Verification

The project builds with the existing Makefile flags (-std=c11 -O2 -Wall -Wextra -Wpedantic).
Both make test suites pass after the change.

## Not implemented yet

- BC2F / BC2T branch behavior;
- GTE command timing and stalls;
- a full PlayStation memory bus;
- KallistiOS / Dreamcast target build;
- a playable CDI.

This is a host-tested CPU milestone, not a playable emulator.
