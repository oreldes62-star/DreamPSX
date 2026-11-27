# SPU voice playback integration — milestone 4.34

The standalone SPU ADPCM decoder is now connected to a deterministic per-voice playback cursor.

## Implemented

- Each of the 24 voices now owns predictor history, a 28-sample decoded cache, a sample index, a block flag byte, and a byte-addressed playback cursor.
- Key-on resets predictor history and cache state, then initializes the cursor from the voice start-address register (8-byte units).
- The voice sample API loads the next 16-byte ADPCM block from the 512 KiB SPU RAM, including safe wraparound at the end of RAM.
- The decoder fills the per-voice 28-sample cache; subsequent requests consume cached samples without decoding the same block again.
- An end-of-sound marker (flag bit 0) deactivates the voice after the final sample in that block.
- Key-off deactivates selected voices and invalidates their cached block.
- Invalid ADPCM blocks deactivate the affected voice instead of committing malformed playback state.

## Test coverage

The SPU test writes a synthetic ADPCM block into SPU RAM through the transfer port, keys on voice 0, verifies decoded sample delivery, consumes all 28 samples, and checks end-of-sound deactivation.

## Current limitations

This is sample retrieval, not yet timed playback. Pitch stepping, ADSR envelope progression, volume scaling, loop-start/repeat handling, interpolation, voice mixing, SPU IRQ, DMA channel 4, and an audio output backend are not implemented. The end marker is handled; loop flags are currently surfaced but not acted upon. No audible output or hardware compatibility is claimed.

## Next steps

Add deterministic pitch accumulation and ADSR state progression, with tests for rate changes and envelope transitions, before implementing the mixer.
