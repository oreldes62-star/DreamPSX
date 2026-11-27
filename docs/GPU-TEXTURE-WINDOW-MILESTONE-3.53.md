# GPU texture-window milestone 3.53

## Implemented

- GP0(E2h) stores five-bit U/V texture-window masks and offsets.
- Texture coordinates are transformed before VRAM sampling using the PS1 mask/offset rule:
  - U' = (U & ~(maskU << 3)) | ((offsetU & maskU) << 3)
  - V' = (V & ~(maskV << 3)) | ((offsetV & maskV) << 3)
- The transform is shared by 4-bit indexed, 8-bit indexed and direct 15-bit texture sampling.
- GPU reset clears all texture-window state.
- Tests cover all three texture depths and reset behavior.

## Validation

The GitHub Actions host workflow must compile the updated core and run all 14 CTest targets. Its result will be checked before this milestone is considered validated.

## Accuracy limits

This is an initial texture-window implementation; hardware-reference comparison remains necessary for edge cases. Gouraud-textured polygons, dithering, display output, DMA, CD-ROM and timers remain incomplete. No Dreamcast/KallistiOS binary or CDI image is produced yet.

## Next work

Implement GP0 Gouraud-textured triangles, including independent color and UV interpolation, then expand GPU command coverage and hardware validation.
