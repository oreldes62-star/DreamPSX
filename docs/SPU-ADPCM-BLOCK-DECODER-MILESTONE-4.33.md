# SPU ADPCM block decoder — milestone 4.33

DreamPSX now has a standalone decoder for one PS1 SPU ADPCM sound block.

## Block format implemented

- 16-byte block.
- Header byte: low nibble shift, high nibble predictor filter.
- Flags byte is returned to the caller without assigning playback/loop behavior yet.
- Fourteen packed data bytes produce 28 signed 4-bit samples, low nibble first.
- Five predictor coefficient pairs are supported:
  - filter 0: (0, 0)
  - filter 1: (60, 0)
  - filter 2: (115, -52)
  - filter 3: (98, -55)
  - filter 4: (122, -60)
- Predictor history is carried across blocks by `PsxSpuAdpcmState`.
- Samples are clamped to signed 16-bit range.
- Invalid filter and shift headers are rejected before predictor state is committed.

## Tests

The SPU test covers nibble order, positive and negative samples, flags propagation, predictor history, and malformed filter/shift headers.

## Scope limits

This is a low-level block decoder only. It is not yet connected to the 24 voice playback cursors. Loop-start/end flags are surfaced as metadata but not interpreted. Pitch stepping, ADSR, interpolation, per-voice looping, volume, mixing, IRQ and DMA channel 4 remain future work. No audible output or hardware compatibility is claimed.

## Next steps

Connect the decoder to an explicit voice playback state with a 28-sample decoded cache, block address progression, and loop metadata. Keep decoding deterministic and independently testable before implementing the mixer.
