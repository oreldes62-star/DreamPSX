# GTE DCPL milestone 3.38

## Implemented

Added DCPL (Depth Cue Color Light, command 0x29).

The command executes the lighting pipeline:
1. Transform the input normal with the light matrix.
2. Apply the color matrix and background color.
3. Modulate the result by the R/G/B channels in RGBC.
4. Interpolate the modulated color toward RFC/GFC/BFC using IR0.
5. Apply sf/lm handling, MAC overflow checks, IR saturation and RGB FIFO output.

The implementation retains the CODE byte from RGBC and updates MAC1-MAC3, IR1-IR3 and the RGB FIFO.

## Regression coverage

The host test uses identity light/color matrices, a known normal and RGB input, and a half-way far-color interpolation. It checks all three MAC outputs and the packed RGB FIFO result.

## Validation scope

GitHub Actions host tests are being checked for this change. This milestone is not a Dreamcast/KallistiOS build and does not demonstrate BIOS boot, CDI generation, Flycast gameplay or GDEMU compatibility.
