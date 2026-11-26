# GTE AVSZ3 / AVSZ4 milestone 3.27

## Implemented

The GTE command dispatcher now supports both average-Z commands:

- AVSZ3 (command 0x2D): sums SZ1, SZ2 and SZ3, multiplies by signed ZSF3.
- AVSZ4 (command 0x2E): sums SZ0, SZ1, SZ2 and SZ3, multiplies by signed ZSF4.
- MAC0 receives the full signed 32-bit result.
- MAC0 positive/negative overflow flags are generated.
- OTZ receives MAC0 shifted right by 12 and saturated to the unsigned 16-bit range.
- OTZ saturation sets FLAG.18; the FLAG summary bit is updated.

## Regression coverage

The host GTE test suite checks:
- AVSZ3 with known SZ values and a unit fixed-point scale.
- AVSZ4 including SZ0 and its separate scale register.
- Negative result clamping to OTZ=0.
- Positive MAC0 overflow and OTZ upper saturation to 0xFFFF.

## Validation

GitHub Actions host build and CTest passed on commit `11c46746343704fff887d3ec83bfed4a73e90aae`:

- Build successful.
- 13/13 test targets passed.
- `test_gte` passed with the new AVSZ3/AVSZ4 cases.

This validates the host implementation only. No Dreamcast/KallistiOS build, PS1 BIOS boot, CDI image, Flycast gameplay or GDEMU test is claimed.

## Next work

Continue with GTE command coverage and reference-based accuracy checks. Candidate commands include SQR, OP and the lighting/color pipelines.
