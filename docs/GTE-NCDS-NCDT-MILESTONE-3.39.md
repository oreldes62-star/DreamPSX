# GTE NCDS and NCDT milestone 3.39

## Implemented

Added the normal-color depth-cue commands:

- NCDS (0x13): processes V0 through the light matrix, background/color matrix, RGBC modulation and far-color interpolation.
- NCDT (0x16): performs the same pipeline for V0, V1 and V2, pushing three RGB results in sequence.

The implementation applies sf shifting, intermediate IR saturation, the unclamped depth-cue difference stage, MAC overflow checks, final IR saturation and RGB FIFO updates. NCDT accumulates FLAG bits across all three vertices and restores the V0 input registers after processing.

## Regression coverage

Host tests verify NCDS MAC/RGB output against a known identity-matrix case. NCDT tests use three distinct normals, verify FIFO order and confirm V0 registers are preserved.

## Validation scope

GitHub Actions host validation is pending for the latest test commit. This does not constitute a Dreamcast/KallistiOS build, BIOS boot, CDI generation, Flycast gameplay test or GDEMU validation.
