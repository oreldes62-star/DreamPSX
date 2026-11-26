# GTE CC milestone 3.41

## Implemented

Added CC (Color Color, command 0x1C):

1. Compute background color plus the color matrix multiplied by IR1–IR3.
2. Apply sf shifting and IR saturation to the intermediate color.
3. Multiply each RGBC channel by the corresponding intermediate IR value, with the GTE's four-bit fixed-point scale.
4. Apply final sf shifting, update MAC1–MAC3 and IR1–IR3, and push the packed result into the RGB FIFO.

The implementation tracks MAC overflow and IR saturation flags and preserves the RGBC code byte.

## Regression coverage

The host-side test uses an identity color matrix and distinct IR values (0x1000, 0x0800, 0x0400) with RGBC 0x00808080. It checks MAC/IR outputs and the packed RGB FIFO result.

## Validation scope

GitHub Actions host validation is pending. It does not constitute a Dreamcast/KallistiOS build, BIOS boot, CDI generation, Flycast gameplay test or GDEMU validation.
