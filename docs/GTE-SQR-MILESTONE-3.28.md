# GTE SQR milestone 3.28

## Implemented

The GTE command dispatcher now supports SQR (command 0x28):

- Squares IR1, IR2 and IR3 independently.
- Writes the shifted or unshifted results to MAC1, MAC2 and MAC3 according to sf.
- Applies the signed/non-negative IR saturation mode selected by lm.
- Sets the FLAG summary bit when a relevant arithmetic or saturation flag is raised.

## Regression coverage

The host GTE tests verify:
- Positive and negative IR inputs.
- sf=1 fixed-point results.
- sf=0 unshifted MAC results.
- IR outputs after saturation.

## Validation

The source and tests are committed. GitHub Actions validation for commit `93334f71f79239f8bd4333ceee65e511e47fc6bb` is pending at the time this document was added. The previous AVSZ3/AVSZ4 milestone passed all 13 host test targets.

This is host-side development only; no Dreamcast/KallistiOS build, BIOS boot, CDI or game compatibility is claimed.

## Next work

Add OP (outer product) and continue toward the lighting/color GTE command family, with hardware-reference comparisons and focused regression tests.
