# GPU Gouraud-textured triangle milestone 3.54

## Implemented

- GP0(34h) command parsing accepts the complete eight-word parameter payload following the command word.
- The GPU packet buffer now holds eight parameter words, covering the interleaved color, XY and UV fields.
- Rasterization reuses the established textured-triangle coverage, UV interpolation, texture-window transform, 4/8/15-bit sampling and CLUT lookup paths.
- Red, green and blue vertex channels are independently interpolated across the triangle and used for per-pixel texture modulation.
- Existing GP0(24h) and GP0(25h) paths retain their previous flat-color/raw behavior.
- A deterministic GPU test checks the interpolated Gouraud color modulating a white 15-bit texel.

## Validation

GitHub Actions validation is required before this milestone is considered complete. Host-side tests do not establish timing accuracy or compatibility on real Dreamcast hardware.

## Accuracy limits

This is the initial opaque Gouraud-textured triangle path. Semi-transparent Gouraud-textured variants, dithering, display output, DMA, CD-ROM and timers remain incomplete. Texture interpolation and modulation still require comparison against hardware references and known-good emulator behavior. No Dreamcast/KallistiOS binary or CDI image is produced yet.

## Next work

Add the semi-transparent Gouraud-textured variant and broaden command-level GPU conformance tests.
