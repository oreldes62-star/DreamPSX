# Flat-shaded triangle milestone 3.45

## Implemented

- GP0 command 20h flat-shaded triangle packet parsing (RGB command word plus three vertex coordinates).
- Signed 11-bit X/Y coordinate decoding.
- Integer edge-function rasterization using pixel-center samples.
- Bounding-box clipping to the 1024 × 512 VRAM surface.
- Both triangle winding directions are handled by the edge-sign test.
- Degenerate triangles are rejected.
- Tests cover basic triangle coverage, outside pixels and negative-coordinate clipping.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `94871c9c603d6ebc7542e8beadc60736b83773ba`.

## Accuracy limits

This is a first deterministic flat-triangle rasterizer, not yet a cycle-accurate or fully hardware-matching PS1 GPU implementation. It currently uses inclusive edge tests rather than the GPU's exact top-left/shared-edge fill convention. Draw-area registers, drawing offsets, dithering, semi-transparency, texture mapping, polygons with Gouraud shading and GPU timing are not implemented.

## Next work

Add GP0 draw-mode and draw-area state, then implement textured and Gouraud-shaded primitives on top of the rasterizer. Validate shared-edge coverage and coordinate offset/clipping behavior against documented PS1 GPU semantics.
