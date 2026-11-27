# GPU raw 15-bit textured triangle milestone 3.49

## Implemented

- GP0(25h) raw textured triangle packet parsing with three XY/UV vertex pairs.
- Direct-color 15-bit texture page selection from the TPAGE field.
- Affine UV interpolation using the same integer edge-function weights as Gouraud triangles.
- Sampling from the selected texture page in VRAM.
- Zero-valued texture pixels are treated as transparent and leave the destination unchanged.
- Reuses signed vertex coordinates, drawing offsets, drawing-area clipping and top-left shared-edge coverage.
- Tests cover texture sampling and transparent texel preservation.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `436dfb86aa8270d766f32d0283eaa19415e00d42`.

## Explicit limitations

Only raw, direct 15-bit texture pages are supported by this first textured path. 4-bit/8-bit indexed textures and CLUT lookup, modulated textured polygons, perspective correction, semi-transparency, dithering and exact hardware texture-window behavior are not implemented. No Dreamcast binary or CDI output is produced yet.

## Next work

Implement indexed 4-bit and 8-bit texture sampling with CLUT lookup and dedicated VRAM layout tests, then add the modulated textured triangle command.
