# PS1 GPU and timer timing model

## Purpose

This note records the timing facts that the machine scheduler must preserve before
replacing its current approximate scanline scheduler. It is a design reference,
not a claim that the current implementation is cycle-accurate.

## Clock domains

The PS1 CPU system clock is nominally 33.8688 MHz. The GPU video clock is a
separate regional clock:

| Video clock region | GPU video clock |
| --- | ---: |
| NTSC | 53.693175 MHz |
| PAL | 53.203425 MHz |

The console's physical video-clock reference is regional. GP1(08h)'s PAL/NTSC
output-mode bit does not itself change the oscillator fitted to a real console.
An emulator therefore needs an explicit emulated hardware-region choice (or a
clearly documented policy) rather than silently treating the output-mode bit as
the physical clock source.

Using the nominal CPU and GPU frequencies, the conversion from CPU cycles to
GPU video clocks is:

| Region | GPU clocks per CPU cycle |
| --- | ---: |
| NTSC | 715909 / 451584 |
| PAL | 709379 / 451584 |

These are rational ratios. Do not round each CPU cycle or each scanline
independently; preserve the fractional phase across calls and scanlines.

## Horizontal timing

The nominal GPU video-clock counts per scanline are:

| Region | Video clocks per scanline |
| --- | ---: |
| NTSC | 3413 |
| PAL | 3406 |

The corresponding *average* CPU cycles per scanline from the nominal clocks
are approximately 2152.866065 NTSC and 2168.227568 PAL. The scheduler must
preserve fractional phase rather than replacing these with fixed integer
line lengths.

GP1(06h) X1/X2 are expressed in GPU video-clock units relative to horizontal
sync, not pixels. The displayed picture occupies the interval [X1, X2);
horizontal blanking is outside that interval. The timer HBlank signal is a
timing signal and must be kept distinct from any TV-output interpretation.

## Timer relationships

- Timer 0 can use the CPU system clock or the GPU dot clock.
- The dot clock is the GPU video clock divided by the horizontal-resolution
  divisor: 10 (256), 8 (320), 7 (368), 5 (512), or 4 (640).
- Preserve both the CPU-to-GPU fractional phase and the dot-divider phase.
  Timer 0 must not lose fractional dot clocks at every API call or scanline.
- Timer 1 can use the CPU system clock or HBlank. One HBlank event is associated
  with each scanline, including vertical blanking.
- Timer 1 VBlank synchronization is driven by the GPU vertical blank signal;
  it is not interchangeable with the display's TV blanking interpretation.
- Timer 2 remains on CPU system clock or CPU system clock divided by eight.

## Implementation plan

1. Introduce an explicit machine video-clock region, separate from GP1(08h)'s
   output-mode bits.
2. Add rational CPU-to-GPU clock accumulation with a persistent remainder.
3. Express scanline phase and GP1(06h) boundaries in GPU video-clock units.
4. Derive Timer 0 dot ticks from GPU clocks and its divisor, preserving both
   remainders.
5. Keep Timer 1 HBlank events and VBlank synchronization in the same ordered
   raster timeline.
6. Add tests for long-run frequency totals, one-block versus chunked advances,
   PAL/NTSC cross-mode combinations, and GP1(06h) custom ranges.
7. Only then tune against external timing tests and real Flycast/hardware
   observations. Passing host tests proves internal consistency, not hardware
   accuracy.

## References

- psx-spx GPU timing notes: https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
- psx-spx timer/root-counter notes: https://psx-spx.consoledev.net/ps1/system/timers/
- psx.wiki display timing measurements: https://psx.wiki/tests/display-timing/
- psx.wiki GPU video timing overview: https://psx.wiki/hardware/gpu/video-timing/
