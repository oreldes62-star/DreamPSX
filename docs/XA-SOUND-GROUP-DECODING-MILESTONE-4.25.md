# XA sound-group decoding milestone — 4.25

## Delivered

- Added decoding of the 128-byte XA sound-group structure.
- Handles the 4-bit layout (8 interleaved sound units) and 8-bit layout (4 units).
- Extracts each unit's predictor/filter and shift parameters.
- Deinterleaves mono and stereo layouts into planar signed 16-bit PCM buffers.
- Mono samples are mirrored into the right output buffer; stereo left/right predictor histories remain independent.
- Validates all sound-unit shift headers before mutating decoder state or output.

## Validation

GitHub Actions host build succeeded; CTest passed **40/40 tests**.

The tests cover silent groups, 4-bit nibble extraction, stereo channel separation, 8-bit mono extraction, invalid shift values, and the lower-level unit decoder.

## Scope and limitations

This milestone decodes one 128-byte sound group, not a complete 2324-byte XA sector stream. The CD-ROM does not yet feed groups into this decoder, and there is no resampling, SPU/CD audio mixer integration, or Dreamcast audio backend. No real-game XA playback or hardware performance is claimed.

## Next step

Walk all 18 sound groups in a Form 2 sector, enforce XA audio subheader and Setfilter file/channel selection, retain per-channel ADPCM history, and prepare the resulting PCM stream for audio scheduling.
