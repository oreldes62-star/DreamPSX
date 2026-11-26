# GTE CDP milestone 3.40

## Implemented

Added CDP (command 0x14), the color depth-cue operation:

1. Apply the background color plus color-matrix transform to IR1–IR3.
2. Modulate the resulting channels by RGBC.
3. Interpolate the modulated color toward the far-color registers using IR0.
4. Write MAC1–MAC3 and IR1–IR3, update FLAG, and push the resulting color into the RGB FIFO.

The command honors sf and lm and performs MAC overflow and IR saturation checks.

## Regression coverage

A host-side identity color-matrix case checks the resulting MAC1–MAC3 values and packed RGB FIFO output.

## Validation scope

Host CI is the validation target. A passing host test does not imply a Dreamcast/KallistiOS build, BIOS boot, CDI image, Flycast gameplay test or GDEMU validation.
