# GTE GPF milestone 3.30

## Implemented

The GTE dispatcher now supports GPF (command 0x3D), the general-purpose fixed-point multiplication operation:

- MAC1–MAC3 are computed from IR0 multiplied by IR1–IR3.
- sf selects the 12-bit fixed-point shift.
- MAC overflow checks are applied to each channel.
- IR1–IR3 are updated with lm-controlled saturation.
- The FLAG summary bit is updated when an arithmetic or saturation flag is raised.

## Regression coverage

The host GTE tests cover:
- IR0=1.0 with sf=1, preserving the input IR channels.
- Positive and negative channel values.
- sf=0, verifying unshifted MAC products.

## Validation

GitHub Actions host build and CTest passed on commit `d772f96e07dbe80466aab1c259b84763c01b4f3f`:

- Build successful.
- 13/13 test targets passed.
- `test_gte` passed with the new GPF cases.

This validates the host implementation only. It does not establish Dreamcast/KallistiOS buildability, PS1 BIOS boot, CDI generation, Flycast gameplay or GDEMU compatibility.

## Next work

Continue with GPL and then build up the GTE color pipeline. Each command will need reference-vector testing before being considered hardware-accurate.
