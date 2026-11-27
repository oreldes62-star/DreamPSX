# Complete XA sector decoder milestone — 4.27

## Delivered

- Added `psx_xa_adpcm_decode_sector()` to decode all 18 XA sound groups from a 2324-byte Form 2 user-data payload.
- Produces planar signed 16-bit PCM buffers for mono or stereo streams.
- Supports 4-bit and 8-bit XA coding layouts.
- Preserves independent left/right predictor histories across sound groups and consecutive sector calls.
- Checks sector length, audio channel count, bit depth, output capacity, and every sound-unit shift header before committing predictor state.
- Predictor state is copied locally and committed only after the entire sector has decoded successfully.
- Corrected XA coding-byte validation: bits 1, 3, 5 and 7 are reserved; bit 6 is the valid emphasis flag and is now exposed in `PsxDiscXaAudioFormat`.

## Output sizes

| Format | Samples per channel per sector |
|---|---:|
| 4-bit mono | 4032 |
| 4-bit stereo | 2016 |
| 8-bit mono | 2016 |
| 8-bit stereo | 1008 |

The decoder leaves the final 20 bytes of the 2324-byte Form 2 payload unused; the 18 sound groups occupy 2304 bytes.

## Validation

GitHub Actions host build succeeded and CTest passed **40/40 test targets**. Tests cover mono/stereo 4-bit decoding, mono 8-bit decoding, expected output lengths, short sectors, insufficient output capacity, malformed headers near the end of a sector, and predictor-state rollback on failure.

## Scope and limitations

This milestone decodes a complete XA sector to PCM buffers, but it is not yet connected to the CD-ROM ReadN/ReadS data flow, Setfilter-driven stream selection, audio timing/resampling, SPU/CD audio mixing, or a Dreamcast audio backend. It does not claim real-game XA playback or hardware performance.

## Technical references

- [PSX-SPX: CD-ROM XA Audio ADPCM Compression](https://problemkaputt.de/psxspx-cdrom-xa-audio-adpcm-compression.htm)
- [vgmstream XA decoder layout notes](https://github.com/vgmstream/vgmstream/blob/24ffd6ce3e1f34fdebf9f5316d18ad2e162838ce/src/coding/xa_decoder.c)

## Next step

Connect decoded XA sectors to CD-ROM sector delivery, applying the programmed file/channel filter and Setmode rules. Then introduce a bounded PCM queue and deterministic audio scheduling before integrating a Dreamcast-specific output backend.
