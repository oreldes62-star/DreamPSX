# GPU indexed texture and CLUT milestone 3.50

## Implemented

- The initial textured-triangle path now supports all three PS1 texture depths: 4-bit indexed, 8-bit indexed and direct 15-bit.
- 4-bit textures unpack one nibble from each 16-bit VRAM word (four texels per word).
- 8-bit textures unpack one byte from each 16-bit VRAM word (two texels per word).
- Indexed texels resolve through the CLUT coordinates encoded in the first UV word.
- Texture page X/Y and depth are decoded from the TPAGE field in the second UV word.
- A zero texel remains transparent and does not overwrite the destination.
- Tests cover packed 4-bit lookup, packed 8-bit lookup and the existing direct 15-bit path.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `6f44821b3abbbf1c8a78972cfec55ec250cc53ce`.

## Accuracy limits

This is an initial affine texture sampler. Exact hardware UV precision, texture-window masking, dithering, semi-transparency and modulated textured polygons remain incomplete. No Dreamcast/KallistiOS binary or CDI image is produced by the host workflow.

## Next work

Implement modulated textured triangles and then semi-transparency modes, with tests for color modulation, blending equations and texture-page state.
