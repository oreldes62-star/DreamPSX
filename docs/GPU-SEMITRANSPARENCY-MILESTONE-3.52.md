# GPU semi-transparency milestone 3.52

## Implemented

- GP0(E1h) stores the semi-transparency blend mode from draw-mode bits 5-6.
- Textured-triangle opcode 25h blends texels carrying the STP bit (bit 15) with the destination pixel.
- All four PS1 blend equations are implemented with per-channel 5-bit saturation:
  - Mode 0: average, (B + F) / 2.
  - Mode 1: additive, B + F.
  - Mode 2: subtractive, B - F.
  - Mode 3: quarter-additive, B + F / 4.
- Texels without STP are copied without blending; transparent zero texels still leave the destination unchanged.
- Tests exercise all four blend modes with deterministic source and destination colors.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `3e4fe77d624bad7d4e68a7203a0ce30292296af1`.

## Accuracy limits

The current implementation applies blending to the textured-triangle path only. Exact hardware behavior for all primitive types, dithering, texture-window masking, Gouraud-textured triangles and hardware-specific rounding still need implementation/reference validation. No Dreamcast/KallistiOS binary or CDI image is produced yet.

## Next work

Add texture-window state (GP0(E2h)) and apply its mask/offset to 4-bit, 8-bit and 15-bit texture coordinates, with wraparound tests.
