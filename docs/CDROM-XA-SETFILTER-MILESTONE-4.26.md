# CD-ROM XA Setfilter milestone — 4.26

## Delivered

- Implemented CD-ROM command `Setfilter` (0x0D), consuming the two parameter bytes in order: XA file number and channel number.
- Added persistent filter file/channel state to the CD-ROM device; reset restores both values to zero.
- Added `psx_cdrom_xa_sector_matches_filter()`, which returns true only when the currently loaded sector has a valid XA audio subheader and its file/channel identifiers match the programmed filter.
- Kept sector data delivery separate from XA audio selection: this milestone exposes the selection decision but does not yet discard nonmatching sectors from the data FIFO or route PCM to an audio device.

## Validation

GitHub Actions host build succeeded and CTest passed **40/40 tests**. Tests cover the default filter mismatch, a wrong channel, and a matching file/channel pair after Setfilter.

## Scope and limitations

This does not yet decode a complete XA sector into PCM or play XA audio. It does not implement Setmode XA routing, SPU transfer, audio timing/resampling, or a Dreamcast audio backend. No retail game playback or hardware compatibility is claimed.

## Next step

Implement a bounded whole-sector XA decoder that walks the verified sound-group layout, checks the subheader's coding parameters, and preserves per-channel predictor state across groups. Then connect it to an audio queue rather than the CD-ROM data FIFO.
