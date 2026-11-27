# GPU raw textured triangle correction milestone 3.59

## Implemented

- Corrected GP0(25h), the raw textured triangle command, to use the opaque path.
- Raw textured texels now bypass semi-transparency blending even when their STP bit is set.
- Added a regression test with an STP-set source texel and a non-default global E1h blend mode.
- Registered the new test in the host CMake/CTest suite.

## Validation

The regression test is committed. A successful host build and complete CTest run are still required before this milestone is considered validated.

## Scope and limitations

This corrects the GP0(25h) dispatch behavior only. GP0(27h) raw semi-transparent textured triangles, hardware-level rasterization fidelity, Dreamcast/KallistiOS integration and physical hardware validation remain outstanding. No Dreamcast binary or CDI image is produced by this milestone.
