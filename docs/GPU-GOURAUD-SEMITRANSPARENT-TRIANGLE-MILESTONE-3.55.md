# GPU Gouraud semi-transparent textured triangle milestone 3.55

## Implemented

- GP0(36h) accepts the eight-word Gouraud-textured triangle packet.
- Vertex RGB channels are interpolated and modulate the sampled 4-bit, 8-bit or 15-bit texture through the existing GP0(34h) path.
- Texels with STP set are blended with the destination using the active GP0(E1h) semi-transparency mode.
- Texels without STP retain the opaque textured output behavior; zero texels remain transparent.
- GP0(34h) remains opaque and unchanged.

## Validation

- Added a deterministic GP0(36h) test using a white STP texel, white Gouraud vertex colors, a red destination pixel and average blending.
- Expected output is 0x3DEF, confirming the texture modulation and STP-controlled blend are both applied.
- GitHub Actions host build and CTest are the required validation gate; this change has not been tested on physical PlayStation or Dreamcast hardware.

## Accuracy limits

This is the initial semi-transparent Gouraud-textured triangle path. Hardware-specific interpolation/rounding, dithering, display output, DMA, CD-ROM, timers and Dreamcast/KallistiOS integration remain incomplete. No CDI or playable game is produced by this milestone.
