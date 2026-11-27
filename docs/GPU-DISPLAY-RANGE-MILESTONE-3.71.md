# GPU display range — milestone 3.71

## Scope

This milestone derives the effective output rectangle from the GP1 horizontal
and vertical display-range registers. It does not implement scanline timing,
HBlank/VBlank scheduling, interlace field sequencing, or a video signal.

## Behavior

- GP1(06h) stores horizontal start/end in video-clock units.
- The horizontal dot-clock divisors are 10, 8, 5, 4 and 7 for 256, 320,
  512, 640 and 368-pixel modes respectively.
- Effective width follows the documented hardware rule:
  `(((X2 - X1) / cycles_per_pixel) + 2) & ~3`.
- GP1(07h) height is the direct line count `Y2 - Y1`; unlike width, it is
  not rounded.
- Empty or reversed ranges are reported as invalid, with zero effective
  dimensions. Overscan ranges are retained rather than clamped to nominal
  mode dimensions.

## API

`PsxGpuDisplayModeInfo` now includes `range_width`, `range_height` and
`range_valid`. The existing `width` and `height` remain the nominal mode
resolution selected by GP1(08h).

## Validation added

The host test covers all five horizontal modes, nominal 240/480-line ranges,
overscan, empty/reversed ranges, and the pre-existing GPUSTAT mode checks.
Tests are registered with CTest but have not been executed in this environment.

## References

- psx-spx, GPU GP1(06h)/(07h), dot-clock and display-range behavior:
  https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
