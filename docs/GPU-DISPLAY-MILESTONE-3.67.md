# GP1 display controller registers — milestone 3.67

## Scope

This milestone introduces persistent display-controller state and decoding for
the core GP1 display commands. It does not yet synthesize a timed video signal
or automatically derive an output framebuffer size.

## Implemented commands

- GP1(00h): GPU reset restores display-register defaults.
- GP1(03h): display enable / disable state and GPUSTAT bit 23.
- GP1(05h): display VRAM origin; horizontal origin is even-aligned.
- GP1(06h): horizontal display range.
- GP1(07h): vertical display range.
- GP1(08h): display mode byte.

The public `PsxGpuDisplay` structure and `psx_gpu_get_display` accessor
allow a host video backend to inspect the decoded state without depending on
internal command parsing.

## Regression coverage

`test_gpu_display` checks reset defaults, coordinate/range decoding,
mode-register masking, enable/disable status behavior, reset restoration and
invalid accessor arguments.

## Remaining work

- Interpret display-mode resolution and color-depth bits.
- Convert the configured display rectangle into a host framebuffer, including
  24-bit mode and VRAM wrapping behavior.
- Model interlace, PAL/NTSC timing and scanout timing.
- Validate the resulting output on an actual frontend such as Flycast.

## Validation status

The test is registered with CTest. No CI run or target build has yet been
confirmed.
