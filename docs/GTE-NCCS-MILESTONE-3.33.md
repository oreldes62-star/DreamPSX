# GTE NCCS milestone 3.33

## Implemented

Added the NCCS (Normal Color Color Single, command 0x1B) execution path.

- Transforms V0 by the light matrix.
- Applies the color matrix and background color.
- Modulates the resulting IR vector by the R, G and B channels in RGBC.
- Applies the command's fixed-point shift and IR saturation.
- Writes MAC1-MAC3 and IR1-IR3, then pushes the result and CODE byte to the RGB FIFO.
- Accumulates MAC overflow and IR saturation flags.

The pipeline follows the PSX-SPX command reference:
https://problemkaputt.de/psxspx-gte-color-calculation-commands.htm

## Regression coverage

The host GTE test checks identity light/color matrices, distinct RGB channels, resulting MAC/IR values, and the RGB FIFO including CODE.

GPL was also corrected: its previous MAC base is shifted left by 12 only when sf=1, before adding IR0*IR and applying the final arithmetic shift. The sf=0 regression expectations now check the unshifted base.

## Validation

GitHub Actions validation is pending for the latest source and test commits. No passing result is claimed until the workflow completes.

This is host-side work only. It does not establish a Dreamcast/KallistiOS build, PS1 BIOS boot, CDI image, Flycast gameplay or GDEMU compatibility.
