# GPU foundation milestone 3.42

## Scope

This milestone starts the GPU subsystem instead of extending the existing CPU/GTE exception test matrix.

## Implemented

- Dedicated `PsxGpu` state with a 1024 × 512 16-bit VRAM buffer.
- GPU initialization and command-state reset.
- GP0 command parser for the fixed-size Fill Rectangle command (02h).
- 24-bit RGB command color conversion to 15-bit VRAM pixels.
- Rectangle origin alignment to 16 pixels and clipping at VRAM edges.
- GP1 reset (00h) and command-buffer reset (01h).
- Read-only GPU status accessor and bounded VRAM pixel inspection API.
- GPU state owned by `PsxMachine`, initialized during machine setup and reset with the machine.
- Host test target covering fill color conversion, rectangle alignment/extent, parser reset, GPU reset and VRAM bounds.

## Validation

The host CMake build and all 14 CTest targets passed in GitHub Actions on commit `81ba7a2b92080317911f2475c3e0fce2164bd36c`.

## Explicit limitations

This is a first GPU core, not a complete PS1 GPU. GP0 polygon, line, texture, blending, display and VRAM transfer commands are not implemented. GP1 display mode and DMA direction controls are not implemented. GPU registers are not yet mapped into the CPU bus, and no Dreamcast/KallistiOS renderer or CDI output exists.

## Next work

1. Map GP0/GP1 MMIO at 1F801810h/1F801814h through the machine bus.
2. Add GPUREAD/GPUREAD status semantics and DMA-ready status bits.
3. Implement VRAM-to-CPU and CPU-to-VRAM transfer commands.
4. Add display timing/output and then basic untextured polygon rasterization.
