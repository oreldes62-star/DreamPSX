# SPU per-voice register bank — milestone 4.32

The SPU now models the register layout for all 24 voices as typed state, rather than treating the entire voice bank as anonymous register storage.

## Per-voice register layout

Each voice occupies 0x10 bytes in the SPU register window:

| Offset | Register | Current model |
|---|---|---|
| +0x0 | Volume left | 16-bit stored value |
| +0x2 | Volume right | 16-bit stored value |
| +0x4 | Pitch | Stored with 14-bit mask |
| +0x6 | Start address | 16-bit SPU RAM address unit |
| +0x8 | ADSR low | Raw 16-bit parameter |
| +0xA | ADSR high | Raw 16-bit parameter |
| +0xC | Current volume | Read-only state, initially zero |
| +0xE | Repeat address | 16-bit SPU RAM address unit |

All 24 banks are represented in `PsxSpuVoice voices[24]`. Reset clears every voice. Tests cover voice 0 and voice 23, independent parameters, pitch masking, and the read-only current-volume register.

## Scope limits

The register values are not yet connected to a sample playback engine. There is no pitch counter, sample start/loop behavior, ADSR state machine, ADPCM decoding per voice, interpolation or mixing. Volume sweep interpretation and exact hardware address scaling remain to be implemented and validated against hardware documentation.

## Validation

CI validation is pending for this milestone. No audible playback or full SPU compatibility is claimed.

## Next steps

Implement ADPCM block decoding for a single voice, then connect voice pitch and ADSR state to deterministic sample stepping and unit tests.
