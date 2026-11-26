# GPU image transfer milestone 3.44

## Implemented

- GP0 A0h CPU-to-VRAM image upload command.
- GP0 C0h VRAM-to-CPU image readback command.
- GPUREAD data reads at `0x1F801810`; GPUREAD is routed through the same bus path as GP0 writes.
- Two 16-bit pixels are packed per 32-bit transfer word, low pixel first.
- Odd pixel counts consume only the final low halfword; the unused upper halfword is ignored/returned as zero.
- Transfer dimensions are decoded from the 10-bit width and 9-bit height fields; zero encodings represent the full VRAM width/height.
- VRAM coordinates wrap at the 1024 × 512 boundary.
- GP1 command-buffer reset cancels an in-progress transfer.
- Tests cover odd-sized upload, readback, GPUREAD through the bus and KSEG1 aliasing.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `b220d2983afcb9a95740c08379cb22224fab0db3`.

## Remaining limitations

- GPU status remains a fixed initial value; FIFO/DMA readiness and IRQ behavior are not modeled.
- GP0 polygons, lines, texture sampling, blending, CLUTs and most drawing commands remain unimplemented.
- GP1 display mode, DMA direction and display timing remain unimplemented.
- No Dreamcast/KallistiOS build, display renderer or CDI output exists yet.

## Next work

Implement a first untextured polygon command (flat-shaded triangle), with correct coordinate sign extension, draw-area clipping, VRAM bounds and deterministic rasterization tests.
