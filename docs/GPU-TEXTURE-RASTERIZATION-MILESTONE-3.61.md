# GPU texture rasterization validation milestone 3.61

## Added

- Added a dedicated host regression target for textured-triangle UV interpolation.
- Verified that interpolated UV coordinates select the expected direct-color texel at a pixel center.
- Covered all four semi-transparency blend modes: average, saturated add, subtract and add-quarter.
- Verified that a texture texel with STP clear bypasses blending and replaces the destination pixel.
- Registered the new target in the CMake/CTest suite.

## Validation status

The regression source and CMake registration are committed. A host compiler/build and CTest execution have not been run from this GitHub-only session; the new test and the complete suite remain unverified at runtime.

## Scope and limitations

This milestone adds regression coverage for existing affine UV interpolation and semi-transparency behavior. It does not claim exact hardware equivalence, perspective-correct mapping, dithering, pixel-clock accuracy, display timing/output, DMA behavior, Dreamcast/KallistiOS integration or physical hardware compatibility.
