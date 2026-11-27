# GPU modulated textured triangle milestone 3.51

## Implemented

- GP0(24h) now parses a modulated textured triangle packet.
- Textured triangles reuse the 4-bit indexed, 8-bit indexed and direct 15-bit texture sampling paths.
- The command RGB color modulates the sampled 5-bit texture channels using the PS1-style 7-bit fixed-point scale, with channel saturation.
- The existing GP0(25h) raw texture path remains available without color modulation.
- Transparent zero texels continue to leave destination pixels unchanged.
- Added a regression test using a white texel and red modulation, expecting a red 15-bit destination pixel.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `3078a04dce15e7468d25f26b8933ffb1607be600`.

## Accuracy limits

The implementation currently covers flat-color modulation on the textured-triangle path. Gouraud-textured triangles, semi-transparency blend modes, texture-window masking, dithering and exact hardware rounding still need implementation and hardware/reference-vector validation. No Dreamcast/KallistiOS binary or CDI image is produced by the host workflow.

## Next work

Implement the four PS1 semi-transparency blend modes and tests for average, additive, subtractive and quarter-additive blending.
