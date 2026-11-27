# GPU display scanout — milestone 3.68

## Scope

This milestone connects the GP1 display origin to a host-provided RGB565
framebuffer. The frontend supplies output dimensions and stride.

## Implementation

- Added `psx_gpu_scanout_rgb565`.
- Reads pixels beginning at GP1(05h)'s configured VRAM origin.
- Wraps X at the 1024-pixel VRAM boundary and Y at the 512-line boundary.
- Converts PS1 RGB555 pixels to host RGB565.
- Respects the GP1(03h) display-disable state.
- Rejects GP1(08h) 24-bit mode until its byte-packed scanout path is added.
- Validates nonzero dimensions, destination pointer and destination stride.

This is a deterministic framebuffer extraction API, not a complete timing
model. The frontend still supplies width and height; horizontal/vertical
range-to-resolution mapping, PAL/NTSC timing, interlace and 24-bit output
remain future work.

## Regression coverage

`test_gpu_scanout` checks disabled output, configured origin, horizontal and
vertical VRAM wraparound, RGB555-to-RGB565 colors, destination row padding,
24-bit rejection and invalid arguments.

## Validation status

The test is registered with CTest. No CI run or target build has yet been
confirmed.
