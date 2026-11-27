# GPU interlace field state — milestone 3.72

## Scope

This milestone adds an explicit odd/even field state for 480-line interlaced
display. It is a state/API foundation, not a scanline or frame timing engine.

## Behavior

- A mode change resets the field state to even and clears GPUSTAT bit 31.
- `psx_gpu_set_display_field` accepts a field only when GP1(08h) selects
  both vertical interlace (bit 5) and 480-line output (bit 2).
- The selected parity is reflected in GPUSTAT bit 31: zero is even, one is
  odd.
- `PsxGpuDisplayModeInfo` exposes `field_valid` and `field_odd`.
- 240-line modes do not accept this 480i field API. In 240-line modes the
  hardware parity signal changes with scanline; that requires a timing core.

## Timing boundary

The frontend/video scheduler must call the setter at each field boundary.
This milestone does not invent a frame clock, generate VBlank interrupts,
advance parity automatically, or filter drawing primitives by field parity.
It also does not change framebuffer scanout: a woven 480-line image remains
read from the complete VRAM frame.

## Validation added

The GPU display-mode test covers mode gating, even/odd transitions, the
GPUSTAT bit 31 mirror, and mode reset behavior. Tests are registered with
CTest but have not been executed in this environment.

## References

- psx-spx GPU display mode and GPUSTAT ODE field behavior:
  https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
- psx.wiki display timing measurements:
  https://psx.wiki/tests/display-timing/
