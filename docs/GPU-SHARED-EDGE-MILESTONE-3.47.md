# GPU shared-edge rasterization milestone 3.47

## Implemented

- Flat-triangle edge tests now use a top-left inclusion convention instead of including all three edges.
- The edge inclusion direction is normalized for both triangle winding orders.
- Shared diagonal pixels are assigned to one triangle only, avoiding double coverage when adjacent triangles are rendered with different colors.
- Added a regression test drawing two triangles that share a diagonal and checking the shared-edge pixel.
- Existing draw-area clipping and signed drawing-offset behavior remain covered by tests.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `54ac0b6cdf7da869a03a9491e1b1c289c7c97ec6`.

## Accuracy limits

This is a deterministic top-left-style edge rule, but full hardware pixel-exact behavior still needs comparison against trusted PS1 GPU test vectors. Gouraud shading, texture mapping, CLUTs, semi-transparency, dithering, display timing and DMA are not implemented. No Dreamcast binary or CDI output is produced by the host test workflow.

## Next work

Add Gouraud-shaded triangle packet parsing and per-vertex color interpolation, retaining the same clipping and edge coverage path.
