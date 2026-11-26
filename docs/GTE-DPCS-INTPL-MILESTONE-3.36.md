# GTE DPCS and INTPL milestone 3.36

## Implemented

Added two far-color interpolation commands.

### DPCS (0x10)
- Starts from the RGB channels in RGBC, scaled by 16 bits.
- Computes the difference to the far-color registers RFC/GFC/BFC.
- Applies IR0 interpolation, sf shifting, MAC overflow checks and IR saturation.
- Pushes the interpolated RGB result with the RGBC CODE byte.

### INTPL (0x11)
- Starts from IR1/IR2/IR3 in 12-bit fixed-point form.
- Interpolates toward the far-color registers using IR0.
- Applies sf shifting, MAC overflow checks and IR saturation.
- Pushes the resulting RGB value with the RGBC CODE byte.

The arithmetic follows the PSX-SPX GTE color-calculation reference and cross-checks against PCSX-Redux tests.

## Regression coverage

Host tests cover halfway interpolation for DPCS (RGB 0x80 toward far color 0x1000) and INTPL (zero vector toward 0x1000), including MAC and RGB FIFO outputs.

## Validation scope

GitHub Actions host validation for the latest test commit is pending. This is not a Dreamcast/KallistiOS build and does not establish BIOS boot, CDI generation, Flycast gameplay or GDEMU compatibility.
