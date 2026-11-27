# GPU textured rectangle milestone 3.65

## Implemented

- Variable-size textured rectangle command family GP0(64h-67h).
- Fixed 8x8 and 16x16 textured rectangle command families GP0(74h-77h) and GP0(7Ch-7Fh).
- E1 draw-mode state retains texture page X/Y, texture depth and semi-transparency mode fields.
- Texture coordinates advance across rectangle pixels and wrap to the 8-bit UV range.
- Existing texture-window, 4-bit/8-bit CLUT and direct 15-bit sampling paths are reused.
- Added regression coverage for variable raw 15-bit rectangles, indexed 4-bit CLUT lookup and fixed-size 8x8 rectangles.

## Validation status

The implementation and CTest target are committed, but no build or test execution has been confirmed in this session.

## Scope

This milestone adds host-side textured rectangle rendering. It does not establish full GPU accuracy or produce a Dreamcast executable/CDI image. Semi-transparent and modulated rectangle variants have command decoding and rendering paths but need dedicated runtime validation.
