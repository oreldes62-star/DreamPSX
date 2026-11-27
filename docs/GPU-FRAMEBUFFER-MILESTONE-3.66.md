# GPU framebuffer conversion — milestone 3.66

## Scope

This milestone adds a small presentation boundary between the PS1 GPU's
15-bit VRAM pixels (RGB555) and a host framebuffer using RGB565.

## Implementation

- Added `psx_gpu_copy_rgb565` to the public GPU API.
- Copies a caller-selected rectangular VRAM region into caller-owned memory.
- Expands the 5-bit green channel to 6 bits by bit replication.
- Supports destination row padding through a pixel-based stride.
- Rejects null pointers, empty rectangles, undersized strides and VRAM
  rectangles that cross the physical VRAM boundary.
- Leaves destination row padding untouched.

This is a format conversion primitive, not yet the complete GP1 display
controller. Display start, horizontal/vertical ranges, display modes,
interlace and timing remain future work.

## Regression coverage

`test_gpu_framebuffer` checks primary colors, white, padded output rows and
invalid arguments / out-of-range rectangles.

## Validation status

The test is registered with CTest. It has not yet been executed in CI or on
a Dreamcast target.
