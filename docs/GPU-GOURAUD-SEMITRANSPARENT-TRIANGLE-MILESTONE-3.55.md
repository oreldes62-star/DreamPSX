# GPU Gouraud semi-transparent textured triangle milestone 3.55

## Implemented

- GP0(36h) accepts the eight-word Gouraud-textured triangle packet.
- Vertex RGB channels are interpolated and modulate the sampled 4-bit, 8-bit or 15-bit texture through the existing GP0(34h) path.
- Texels with STP set are blended with the destination using the semi-transparency mode encoded in the primitive's TPAGE field (bits 5-6 of the second vertex UV word). This per-primitive mode takes precedence over the global GP0(E1h) draw-mode state.
- Texels without STP retain the opaque textured output behavior; zero texels remain transparent.
- GP0(34h) remains opaque and unchanged.

## Validation

- Added deterministic GP0(36h) tests for all four TPAGE semi-transparency modes while deliberately setting GP0(E1h) to a different mode, plus an STP-clear texel that must bypass blending.
- The tests verify Gouraud modulation, destination blending and STP gating together.
- GitHub Actions host build and CTest are the required validation gate; this change has not been tested on physical PlayStation or Dreamcast hardware.

## Accuracy limits

This is the initial semi-transparent Gouraud-textured triangle path. The TPAGE blend-mode selection is now covered independently of the global draw-mode register. Hardware-specific interpolation/rounding, dithering, display output, DMA, CD-ROM, timers and Dreamcast/KallistiOS integration remain incomplete. No CDI or playable game is produced by this milestone.
