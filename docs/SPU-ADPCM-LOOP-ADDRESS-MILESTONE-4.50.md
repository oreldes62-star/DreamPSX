# SPU ADPCM loop-address milestone 4.50

## Scope

This milestone verifies both supported ways to supply a voice's ADPCM repeat address:

- An ADPCM block with flag bit 1 set (0x02) captures that block's address, in 8-byte units, into the voice repeat-address register.
- A game may program the voice repeat-address register directly. An end block with flag bits 0 and 2 set (0x05) uses that programmed address even if no loop-start block has appeared in the stream.

When an end+repeat block is exhausted, the voice remains active and the next source-sample request fetches from the selected repeat address. An end block without repeat still ends the voice.

## Regression coverage

The host SPU tests verify that:

1. A loop-start block followed by an end+repeat block returns playback to the captured block address and leaves the voice active.
2. A directly programmed repeat address is preserved and used by an end+repeat block that has no loop-start marker.

The tests run as part of the repository's build-and-test GitHub Actions workflow.

## Limitations

This is a focused software-model milestone, not proof of cycle-accurate SPU behavior. Hardware-accurate transfer timing, IRQ behavior, reverb, noise, pitch modulation, CD audio mixing, and validation on Flycast or a physical Dreamcast remain outstanding.
