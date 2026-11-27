# SPU key-on/key-off and control foundation — milestone 4.31

This milestone extends the SPU register/RAM model with the first voice-control semantics and corrects 32-bit accesses to the sample transfer port.

## Implemented

- 24-bit active voice mask for the PS1's 24 voices.
- Key-on low/high write registers at offsets 188h/18Ah set voices 0–23 active.
- Key-off low/high write registers at offsets 18Ch/18Eh clear selected active voices.
- Key-on/key-off registers are modeled as write-only; reads return zero.
- SPU control register at offset 1AAh is stored and read back.
- Reset clears active voice state and control.
- CPU 32-bit reads/writes to the transfer-data port now perform two consecutive 16-bit transfers through the same port, rather than incorrectly accessing the adjacent register.
- Host tests cover key-on/off masks, control readback, reset, and CPU-bus access.

## Scope limits

The active-voice mask is control state only. It does not yet synthesize samples or represent per-voice pitch, volume, ADSR, ADPCM predictor history, loop state, or sample playback. SPU status and control side effects remain incomplete; DMA channel 4, IRQ generation, reverb, noise, and audio output are not implemented.

## Validation

The CI workflow is validating this milestone. Do not interpret active voice bits as audible voice playback.

## Next steps

1. Validate the register behavior with CI.
2. Model the per-voice register bank (volume, pitch, start address, ADSR, repeat address).
3. Add deterministic voice state and ADPCM block decoding.
4. Implement envelopes and mix output only after per-voice decoding is covered by tests.
