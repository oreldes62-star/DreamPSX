# GPU display scanout — milestone 3.69

## Scope

This milestone adds packed 24-bit display extraction to the host RGB565
framebuffer API.

## Implementation

- `psx_gpu_scanout_rgb565` now supports both 15-bit and 24-bit display modes.
- 15-bit mode reads RGB555 words and expands green to six output bits.
- 24-bit mode reads packed R, G, B bytes from VRAM and converts them to RGB565.
- Byte reads wrap at the 2048-byte VRAM scanline boundary, including pixels
  whose three components straddle the end of a VRAM row.
- Each output scanline advances to the next VRAM row, beginning at the
  configured GP1(05h) X offset.
- Display-disable handling, destination stride validation and dimension bounds
  remain enforced.

The frontend still supplies output dimensions. GP1 horizontal/vertical timing
is not yet translated into a hardware-accurate output size. PAL/NTSC timing,
interlace and reverse display are also not implemented.

## Regression coverage

`test_gpu_scanout` now covers packed 24-bit RGB byte order, RGB565 conversion,
pixels crossing the final VRAM columns, independent next-row scanout, disabled
output, 15-bit output, stride padding and invalid arguments.

## Validation status

The test is registered with CTest. No CI run or target build has yet been
confirmed; the new test has not been reported as executed.
