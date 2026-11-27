# GPU monochrome rectangle milestone 3.64

## Implemented

- GP0 60h variable-size monochrome rectangles.
- GP0 68h 1x1, 70h 8x8 and 78h 16x16 monochrome rectangles.
- Rectangle coordinates use the existing signed 11-bit coordinate decoding and drawing offset.
- Rasterization is clipped to the inclusive GP0(E3h/E4h) drawing area.
- Added regression coverage for variable and fixed dimensions, clipping sentinels, and signed offsets.

## Validation status

The implementation and CTest target are committed. The test suite has not been executed in this session; runtime validation remains pending.

## Scope

This milestone adds basic untextured rectangle primitives to the host GPU renderer. It does not yet implement textured rectangles, semi-transparent rectangle variants, display output, DMA, CD-ROM, or a Dreamcast executable.
