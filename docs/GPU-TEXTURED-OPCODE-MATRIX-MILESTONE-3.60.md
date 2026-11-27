# GPU textured triangle opcode matrix milestone 3.60

## Implemented

- Added GP0(26h): modulated, semi-transparent textured triangles.
- Added GP0(27h): raw, semi-transparent textured triangles.
- Added GP0(35h): Gouraud-shaded raw textured triangles.
- Added GP0(37h): Gouraud-shaded raw, semi-transparent textured triangles.
- Kept GP0(24h), (25h), (34h) and (36h) on their existing modulation/transparency paths.
- Added a dedicated host regression target covering modulation, raw texel preservation, STP blending and Gouraud command parsing.

## Validation status

The source changes, test source and CMake registration are committed. A host build and complete CTest run have not yet been executed in this environment; CI confirmation is still required.

## Scope and limitations

This closes the currently implemented textured-triangle opcode dispatch matrix for triangle commands 24h–27h and 34h–37h. It does not establish exact GPU hardware fidelity, perspective-correct texture mapping, dithering, display timing/output, DMA behavior, Dreamcast/KallistiOS integration or physical hardware compatibility. No playable Dreamcast build or CDI image is produced by this host-side milestone.
