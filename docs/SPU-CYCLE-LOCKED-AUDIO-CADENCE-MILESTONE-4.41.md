# Cycle-locked SPU audio cadence — milestone 4.41

## Scope

The machine shared system-clock advancement now schedules SPU stereo output at 44,100 frames per second using the PS1 master-clock ratio of 33,868,800 cycles per second / 44,100 = 768 cycles per output frame.

A persistent phase remainder is kept in `PsxMachine.spu_audio_cycle_phase`. This makes the result independent of how elapsed cycles are divided among calls. Each due frame is rendered through the existing SPU mixer and appended to the machine-owned audio queue.

## Validation

Host CI passes 42/42 tests. New machine tests check the exact 767+1 cycle boundary, the one-frame due event, a 1,537-cycle interval, and equivalence between a single interval and split intervals. The existing machine integration tests continue to pass.

## Limitations

This is a simplified fixed-rate output clock. The SPU voices are still advanced by the host render calls rather than by a fully modeled hardware SPU cycle engine. XA CD audio is not mixed into this output queue. There is no AICA/KallistiOS backend, DMA/callback integration, resampling, or real Dreamcast playback validation.
