# GPU HBlank and Timer1 integration — milestone 3.77

## Scope

Connect the existing scanline scheduler to the Timer1 HBlank clock domain without pretending that the current line-at-a-time scheduler models exact horizontal timing.

## Changes

- `PsxGpuVideoState.hblank_pulse` reports one abstract HBlank clock event whenever the frontend advances one scanline.
- `psx_machine_video_advance_scanline` forwards that event as one `PSX_TIMER_CLOCK_HBLANK` tick to Timer1.
- Timer1 continues to use the clock-source selection and IRQ generation implemented in milestone 3.76.
- Host tests check the GPU event, Timer1 increment, IRQ source 5 delivery, acknowledgement, and one-shot behavior.

## Timing model and limitations

This is a deterministic line-level approximation. The event is a pulse associated with each call that advances a scanline; it does not represent the physical start/end phase or duration of horizontal blanking. No dot-clock events are generated yet. Timer0 dot clock, Timer synchronization modes, the Timer2 divider phase, CPU-cycle scheduling, and sub-scanline timing remain future work.

## Validation

Tests were extended and registered in CMake. They have not been executed in this development environment; no build or runtime success is claimed.
