# XA timed PCM scheduling milestone — 4.29

## Delivered

- Added a deterministic XA playback clock based on the PS1 system clock (33,868,800 Hz).
- XA frame deadlines are accumulated as integer clock-rate products with a 64-bit fractional remainder. Splitting a cycle interval across calls does not change the result.
- Supports the decoded XA sample rates (18,900 Hz and 37,800 Hz) without floating-point timing drift.
- Added `psx_cdrom_advance_xa_audio_cycles()` and `psx_cdrom_read_xa_audio_timed()`. The timed reader only returns frames whose playback deadlines have elapsed.
- Added underrun accounting in `xa_audio_underrun_frames`; elapsed frames that have no PCM available are counted and are not replayed late.
- All PCM drain paths keep the scheduled-frame budget synchronized with the ring occupancy.
- Connected the XA scheduler to `psx_machine_advance_system_cycles()`, so CPU instruction timing advances the XA playback clock through the shared machine clock.
- Stream reset clears the fractional phase, deadline budget and underrun counters.

## Timing example

At 18,900 Hz, one frame is due every 1,792 PS1 system cycles. The test advances 1,791 cycles (zero frames due), then one additional cycle (one frame due). This is also tested through the machine-level shared clock.

## Memory

The bounded PCM ring uses 8,192 stereo frames plus one sector of contiguous tail storage for safe wrap decoding. Its two signed 16-bit channel buffers reserve 48,896 bytes in the CD-ROM state structure.

## Validation

GitHub Actions host build succeeded and CTest passed **40/40 test targets**. Coverage includes:
- fractional-cycle accumulation across split calls;
- exact 18.9 kHz frame cadence;
- machine-level clock integration;
- timed PCM draining;
- underrun accounting;
- existing XA decode, file/channel filtering and bounded-queue tests.

## Scope and limitations

This milestone schedules XA PCM frames against PS1 system cycles, but it does not yet resample to the Dreamcast output rate, feed the PS1 SPU CD-input path, mix CD audio with SPU voices, or produce audible output on Dreamcast. It is not a hardware-validated audio pipeline. Rate changes while old frames remain queued are conservatively dropped and counted rather than mixing differently timed sample rates in one queue.

## Next step

Implement explicit rate-segment handling or a controlled stream-boundary flush, then build the PS1 SPU CD-audio input interface and a deterministic resampler/output stage. Validate queue wrap, long-duration clock drift, rate transitions, and underrun recovery before connecting a Dreamcast audio backend.
