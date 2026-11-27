# XA ADPCM decoder foundation — milestone 4.24

## Delivered

- Added `PsxXaAdpcmState` with two predictor-history samples.
- Added a bounded decoder for one 28-sample XA ADPCM sound unit, accepting already-unpacked signed sample codes.
- Supports the 4-bit and 8-bit residual scaling paths, four XA predictor filters, per-channel history supplied by the caller, and signed 16-bit saturation.
- Added explicit validation for unit length, sample depth, filter index, shift range, and 4-bit code range.
- Added reset support and a dedicated host test target.

## Validation

GitHub Actions host build succeeded and CTest passed **40/40 tests**.

## Scope and limitations

This milestone is a decoding primitive, not yet a complete XA sector audio pipeline. It does not unpack the interleaved XA sound-unit layout from 2324-byte sectors, route mono/stereo channels, resample audio, or submit samples to a Dreamcast audio backend/SPU. The caller must preserve one decoder state per audio channel. No real-game XA audio playback or Dreamcast hardware behavior is claimed.

## Next step

Implement and test the XA sector sound-unit deinterleaver, using the XA subheader coding metadata to select channels, sample depth and rate, then connect decoded samples to the SPU/audio output path.
