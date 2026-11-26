# GTE NCS milestone 3.32

## Implemented

Added the NCS (Normal Color Single, command 0x1E) execution path.

- Reads V0 as the input normal.
- Applies the light matrix (LLM), with fixed-point shift and IR limiting.
- Applies the color matrix (LCM) plus background color (BK).
- Writes MAC1–MAC3 and IR1–IR3 from the color stage.
- Pushes the calculated RGB value and RGBC CODE byte into the color FIFO.
- Tracks MAC overflow, IR saturation and the FLAG summary bit.

The arithmetic follows the documented NCS pipeline: light-matrix transform, then background-plus-color-matrix transform. See the PSX GTE command reference at https://psx.wiki/hardware/gte/commands/ and the PSX-SPX specification.

## Regression coverage

The host GTE test suite exercises NCS with identity light and color matrices, checks all three MAC/IR outputs, and verifies the RGB FIFO channels and CODE byte.

## Validation

GitHub Actions host build and CTest passed on commit `9b7cab61bc3a51bde2b92393561752f755c49a2b`:

- Build successful.
- 13/13 test targets passed.
- `test_gte` passed with the NCS regression case.

This is host-side validation only. It does not claim a Dreamcast/KallistiOS build, PS1 BIOS boot, CDI generation, Flycast gameplay or GDEMU compatibility.

## Next

Expand the color pipeline with NCCS and depth-cued variants, adding independent test vectors for intermediate fixed-point stages and FIFO behavior.
