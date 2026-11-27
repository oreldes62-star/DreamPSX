# GPU video timing — milestone 3.73

## Scope

Adds a deterministic scanline-step scheduler with distinct NTSC and PAL
vertical timing. The caller advances it once per emulated scanline. This is
not yet tied to R3000A CPU cycles or the system interrupt controller.

## Timing model

- NTSC progressive: 263 scanlines per frame.
- PAL progressive: 314 scanlines per frame.
- NTSC 480i: alternating 262- and 263-line fields.
- PAL 480i: alternating 312- and 313-line fields.
- Interlaced 480i toggles field parity at field boundaries; a complete frame
  is counted after the odd field returns to even.
- GP1(07h) visible range determines active lines. Lines outside that interval
  are reported as VBlank and mirrored to GPUSTAT bit 10.
- A GP1(08h) mode change resets the raster position and frame counter.

The NTSC/PAL timing values and interlaced field alternation are based on
measured PS1 GPU timing documentation:
https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
https://psx.wiki/tests/display-timing/

## API

`psx_gpu_video_advance_scanline` advances one line and fills
`PsxGpuVideoState` with scanline, field length, frame counter, standard,
field parity, VBlank state and edge events.

## Deliberate limitations

This milestone does not model half-line phase, fractional line timing,
region-specific oscillator drift, CPU-cycle integration, HBlank, Timer0/1
signals, VBlank IRQ0 delivery, or display field line weaving. The physical
console clock region is distinct from the GP1 PAL/NTSC output mode; the
scheduler currently follows GP1 mode for deterministic content timing.

## Tests

A new CTest target checks NTSC/PAL progressive frame lengths, interlaced
field alternation, VBlank entry/exit, status bit 10 and frame counters.
Tests have been added but not executed in this environment.
