# GPU display mode decoding — milestone 3.70

## Scope

This milestone decodes GP1(08h) display mode flags for frontends and keeps
the corresponding GPUSTAT mode bits synchronized.

## Implementation

- Added `PsxGpuDisplayModeInfo` and
  `psx_gpu_get_display_mode_info`.
- Horizontal resolution decoding: 256, 320, 512, 640, or 368 pixels when HDS2
  is selected.
- Vertical mode decoding: 240 or 480 lines.
- Exposes PAL/NTSC, 15/24-bit color depth, interlace and reverse flags.
- GP1(08h) now mirrors HDS2, horizontal/vertical resolution, PAL, color depth,
  interlace and reverse into their documented GPUSTAT bit positions.

The decoded dimensions describe the selected display mode. They do not yet
derive the exact visible width/height from GP1(06h)/(07h) timing ranges.
Interlace field composition and PAL/NTSC scan timing remain future work.

## Regression coverage

`test_gpu_display_mode` checks reset defaults, 368x480 mode flags, GPUSTAT
mirrors, 512-pixel horizontal resolution and invalid API arguments.

## Validation status

The test is registered with CTest. No CI run or target build has yet been
confirmed.
