# R3000A Variable Shifts — Milestone 3.16

## Implemented

- SLLV (SPECIAL funct 0x04): logical left shift using the low five bits of rs.
- SRLV (SPECIAL funct 0x06): logical right shift using the low five bits of rs.
- SRAV (SPECIAL funct 0x07): arithmetic right shift using the low five bits of rs.
- The fixed shamt field must be zero for all three variable-shift encodings;
  malformed encodings are rejected as Reserved Instruction.
- Arithmetic right shift uses the existing portable helper, avoiding
  implementation-defined signed right-shift behavior in C.

## Regression coverage

The CPU execution tests cover shift amounts 0, 1, 31 and 32 (which is masked
to zero), using a source value with its sign bit set. They verify the distinct
logical-left, logical-right and sign-extending arithmetic-right results.

## Validation

GitHub Actions host build and CTest passed on commit
`0d942667d473339401bc7966e6aab68811f42263`:

- CMake configure: passed
- Host build: passed
- CTest: 12/12 tests passed, including `test_cpu_execution`

This is host-side validation only. It is not a Dreamcast/KallistiOS target
build, BIOS boot, CDI image, or gameplay test.

## Limitations

This milestone adds the three MIPS I variable-shift instructions only. It does
not complete the R3000A instruction set or add cycle-accurate timing.
