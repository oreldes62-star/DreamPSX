# CD-ROM XA PCM routing milestone — 4.28

## Delivered

- Added XA audio routing to the CD-ROM sector-loading path.
- Honors Setmode bit 6 (XA-ADPCM enable) and bit 3 (XA file/channel filter).
- Only sectors marked both audio (submode bit 2) and realtime (submode bit 6) are candidates for the XA audio path.
- Matching XA sectors are decoded from their 2324-byte Form 2 payload into signed 16-bit planar PCM.
- The CD-ROM exposes `psx_cdrom_read_xa_audio()` to drain left/right frames and obtain the queued sample rate.
- Added a bounded queue of 8192 stereo frames. When there is insufficient room, the sector is consumed but dropped and `xa_dropped_sectors` increments; the CD-ROM does not overwrite unread audio.
- The queue uses contiguous tail storage for decoding, avoiding large temporary PCM arrays on the call stack.
- Predictor history is kept separately for left and right channels and reset when media, Setfilter selection, or XA-related Setmode bits change.
- With XA filtering enabled, audio+realtime sectors that do not match the programmed file/channel pair are suppressed from the CPU data FIFO. When filtering is enabled but XA decode is disabled, matching audio+realtime sectors are also withheld from the CPU data FIFO.

## API

`psx_cdrom_read_xa_audio(cdrom, left, right, max_frames, &frames_read, &sample_rate_hz)`

- Reads up to `max_frames` queued stereo frames.
- Returns the number of frames actually copied.
- Returns sample rate 0 when the queue becomes empty.
- The consumer is responsible for scheduling/resampling and sending PCM to an audio device.

## Validation

GitHub Actions host build succeeded and CTest passed **40/40 test targets**. New integration coverage checks:
- XA-enabled sector bypasses the CPU data FIFO;
- decoded mono PCM and 18.9 kHz metadata;
- channel filtering prevents a mismatched sector from reaching either output;
- draining the PCM queue returns the expected frame count.

## Scope and limitations

This milestone provides CD-ROM-to-PCM queue routing, not audible game playback yet. There is no SPU CD audio input implementation, no CD-ROM/SPU mixing, no resampler or hardware-accurate XA sector scheduling, no IRQ timing model for XA audio, and no Dreamcast audio backend. Queue overflow currently drops a whole sector and is exposed through `xa_dropped_sectors`. Real-disc/game compatibility and Dreamcast performance have not been validated.

## Technical reference

- [PSX-SPX — CD-ROM control commands and XA sector delivery](https://problemkaputt.de/psxspx-cdrom-control-commands.htm)
- [PSX-SPX — CD-ROM XA audio commands](https://problemkaputt.de/psxspx-cdrom-cd-audio-commands.htm)

## Next step

Introduce a timing-aware XA audio consumer and the PS1 SPU CD audio input path, then connect that path to the Dreamcast audio backend. Keep the CD-ROM queue bounded and add tests for ring wrap, overflow, stream changes, and sample-rate transitions.
