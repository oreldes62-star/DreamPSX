# GPU MMIO and external VRAM milestone 3.43

## Implemented

- Connected the GPU to the CPU-facing PS1 bus.
- GP0 writes are routed at physical address `0x1F801810`.
- GP1 writes are routed at physical address `0x1F801814`.
- GP1 status reads are available at `0x1F801814`.
- KSEG aliases are normalized by the existing bus mapping, so the GPU ports also work through KSEG1 (for example `0xBF801810` and `0xBF801814`).
- Added bus-level tests for GP0 rectangle fill, GP1 status reads and machine-level integration.
- Moved the 1024 × 512 × 16-bit VRAM backing store out of `PsxGpu` and `PsxMachine`. The owner now supplies a 1 MiB VRAM buffer to `psx_gpu_init` / `psx_machine_init`, avoiding a large embedded machine object and stack allocation on Dreamcast.
- Updated all host tests to provide external VRAM storage.

## Validation

GitHub Actions host build and all 14 CTest targets passed on commit `e20b6225a6c78b561a9e00fe039b6b619d6d2d0a`.

## Remaining GPU limitations

- GPUREAD data-port reads at `0x1F801810` are not implemented.
- GP0 polygon, line, texture, blending and VRAM transfer commands are not implemented.
- GP1 display mode, DMA direction and display timing controls are not implemented.
- GPU status currently returns the initial status value; dynamic FIFO/DMA readiness and interrupt flags are not modeled.
- No Dreamcast/KallistiOS renderer or CDI output exists yet.

## Next work

Implement the GP0 image-load (CPU-to-VRAM) and image-store (VRAM-to-CPU) transfer paths, including GPUREAD data buffering and transfer boundary tests. Then expand primitive rasterization.
