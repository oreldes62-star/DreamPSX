# SPU ADSR envelope engine — milestone 4.36

This milestone adds an independently ticked ADSR envelope generator for each of the 24 SPU voices.

## Implemented

- Per-voice envelope level, divider counter and phase: OFF, ATTACK, DECAY, SUSTAIN and RELEASE.
- ADSR1 decoding: attack rate and mode, decay rate and sustain target.
- ADSR2 decoding: sustain rate, direction and mode, plus release rate and mode.
- Rate-derived step and divider increment, including the exponential increase/decrease adjustments.
- Attack saturates at 0x7FFF; decay clamps at the programmed sustain level; release reaches zero.
- Key-On resets envelope level and begins Attack.
- Key-Off enters Release and leaves a nonzero voice active until the envelope reaches zero.
- ENVX reads the current envelope level.
- `psx_spu_voice_envelope_tick()` advances one modeled 44.1 kHz envelope tick. The machine clock is not yet wired to this API.

## Validation

The SPU test covers the fastest linear attack, full-scale saturation, ENVX readback, Key-Off transition, release progression and voice deactivation at zero. GitHub Actions passed all 41 CTest targets.

## Limitations

This is an isolated envelope primitive, not yet integrated into the DreamPSX audio output path. The emulator does not yet schedule envelope ticks from the SPU clock, scale samples by envelope and voice volumes, mix 24 voices, implement volume sweeps, interpolation, noise, reverb, SPU IRQ/DMA, or a Dreamcast audio backend. ADSR timing and corner cases still require comparison against hardware/reference test ROMs.

## References

- psx-spx SPU specification: https://psx-spx.consoledev.net/soundprocessingunitspu/
- psx.wiki ADSR overview: https://psx.wiki/hardware/spu/adsr/
