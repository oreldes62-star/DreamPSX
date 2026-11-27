# GPU textured shared-edge validation milestone 3.62

## Added

- Added a host regression test that draws a square as two raw, semi-transparent textured triangles.
- Checks every pixel in the square to detect uncovered pixels.
- Uses a destination color and an STP texture texel whose single-blend result is known, so a shared-edge double blend is detected.
- Adds a reversed-winding regression check for a textured triangle.
- Registers the new test with CMake/CTest.

## Validation status

The test source and CMake registration are committed. The host build and CTest suite have not been executed in this session, so the assertions are regression coverage awaiting runtime confirmation.

## Scope

This milestone tests the current textured-triangle edge inclusion path. It does not claim complete GPU accuracy, subpixel hardware equivalence, dithering, display timing, DMA, CD-ROM, or Dreamcast hardware compatibility.
