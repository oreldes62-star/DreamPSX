# SPU pitch-driven voice stepping — milestone 4.35

This milestone adds a deterministic pitch phase accumulator to each SPU voice and a rendered-sample API.

## Implemented

- Each voice keeps a 12-bit fractional pitch phase and its current decoded sample.
- The first render request fetches the first decoded ADPCM sample.
- For each subsequent output tick, the voice adds its programmed pitch (register value, masked to 14 bits) to the phase accumulator.
- Every accumulated 0x1000 units advances one source ADPCM sample.
- Rates below 1.0x hold the current source sample for additional output ticks; rates above 1.0x may consume multiple source samples in one output tick.
- Key-on resets pitch phase and held-sample state; key-off invalidates the held sample.

## Test coverage

The SPU test verifies 0.5x stepping (a source sample is held for two output ticks) and 2.0x stepping (two source samples are consumed for one output tick), then verifies key-off.

## Scope and limitations

This is nearest-sample pitch stepping, not interpolation. It does not establish a hardware-accurate SPU output clock, resampling filter, ADSR envelope, volume scaling, voice mixing, loop handling, IRQ/DMA behavior, or audible audio output. Pitch stepping is exposed as a deterministic per-voice primitive for a later mixer.

## Next step

Implement and test the ADSR envelope state machine independently before combining pitch, envelope, volume and all active voices in a mixer.
