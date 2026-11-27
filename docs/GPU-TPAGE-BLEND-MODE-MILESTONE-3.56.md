# GPU TPAGE blend-mode validation milestone 3.56

## Implemented

- Textured triangle semi-transparency now selects its blend equation from TPAGE bits 5-6 carried by the second vertex UV word.
- The per-primitive TPAGE mode takes precedence over the global GP0(E1h) draw-mode value.
- GP0(36h) tests cover all four blend equations with deliberately conflicting E1h state, plus STP-clear texels bypassing blending.

## Validation

- The existing 14-target host CMake/CTest suite remains the required CI gate.
- This milestone has not been validated on physical PlayStation or Dreamcast hardware.

## Accuracy limits

This is a host-side behavior correction, not a complete GPU implementation. Display timing/output, DMA, dithering, hardware-accurate rasterization edge cases, Dreamcast/KallistiOS integration and physical hardware validation remain outstanding. No CDI or playable game is produced by this milestone.
