# GPU drawing area and offset milestone 3.46

## Implemented

- GP0(E3h) sets the inclusive drawing-area top-left coordinate.
- GP0(E4h) sets the inclusive drawing-area bottom-right coordinate.
- GP0(E5h) sets signed 11-bit X/Y drawing offsets.
- Flat-shaded triangle vertices are translated by the configured drawing offset before rasterization.
- Triangle bounding boxes are clipped against the configured drawing area instead of only the full VRAM extent.
- GPU reset restores the default drawing area (0,0)-(1023,511) and zero drawing offset.
- Tests cover drawing-area clipping, offset translation and preservation of pixels outside the newly drawn region.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `b0310378fb2bcfc2e62e2aa1de0b12900e602baf`.

## Accuracy limits

The rasterizer still uses inclusive edge tests, not the PS1 GPU's exact shared-edge fill convention. GP0(E1h) draw mode, texture window, dithering, semi-transparency, textured/Gouraud primitives, display timing and DMA behavior are not implemented. This is host-side validation only; no Dreamcast binary or CDI is produced yet.

## Next work

Implement a consistent top-left/shared-edge rule and then expand to Gouraud-shaded and textured triangles, with additional edge and clipping tests.
