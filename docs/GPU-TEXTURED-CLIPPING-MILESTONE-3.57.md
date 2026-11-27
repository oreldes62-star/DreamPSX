# GPU textured triangle clipping milestone 3.57

## Implemented

- Added a dedicated host regression target for textured-triangle drawing-area clipping.
- The test submits a textured triangle with a non-zero signed drawing offset and verifies that a covered pixel inside the inclusive drawing area is rendered.
- Sentinel pixels outside the drawing area are checked to ensure the textured primitive does not overwrite them.
- Registered the new test in the host CMake/CTest suite.

## Validation

- The new target is intended to run with the existing host CMake/CTest suite.
- GitHub Actions must confirm the full suite after this commit sequence.
- This remains host-side validation; it does not establish physical PlayStation or Dreamcast compatibility.

## Accuracy limits

The test covers the existing textured-triangle clipping and offset path only. It does not validate hardware-accurate rasterization, display output/timing, DMA, dithering, Dreamcast/KallistiOS integration or a playable CDI build.
