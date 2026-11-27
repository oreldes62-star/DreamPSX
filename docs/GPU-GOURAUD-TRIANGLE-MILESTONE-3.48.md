# GPU Gouraud triangle milestone 3.48

## Implemented

- GP0(30h) Gouraud-shaded triangle packet parsing: initial command color/XY followed by alternating vertex color and XY words.
- Per-channel RGB interpolation across the triangle using integer edge-function barycentric weights at pixel centers.
- Conversion of interpolated 24-bit RGB values to 15-bit VRAM pixels.
- Reuses signed 11-bit vertex decoding, drawing offsets, inclusive draw-area clipping and top-left shared-edge coverage.
- Added a deterministic RGB-corner test checking the interpolated center pixel.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `596fc3493a71d5171f24af2fc21f95f6715a42b6`.

## Accuracy limits

This is an initial host-side Gouraud triangle path. Exact hardware rounding, subpixel behavior and edge rules still require comparison against known PS1 GPU test vectors. Textured polygons, CLUTs, semi-transparency, dithering, display timing and DMA are not implemented. No Dreamcast binary or CDI output is produced yet.

## Next work

Add textured triangle packet parsing and texture-page/CLUT sampling, starting with a minimal 4-bit or 8-bit texture path and focused VRAM layout tests.
