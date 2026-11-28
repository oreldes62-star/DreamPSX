# Dreamcast AICA stream buffer byte contract — milestone 4.58

## Objective

Make the KallistiOS stream allocation size and callback byte units explicit and keep them tied together by a host regression test.

## Changes

- Corrected the Dreamcast audio bridge header: KallistiOS callback request and returned lengths are total interleaved byte counts, not frame counts.
- Renamed callback parameter identifiers to `bytes_requested` and `bytes_received` in the public declaration and KOS adapter to prevent unit confusion.
- Added `PSX_KOS_AUDIO_STREAM_BUFFER_BYTES` as the single named 2048-byte allocation value passed to `snd_stream_alloc()`.
- Documented that a 2048-byte interleaved stereo PCM16 block represents 512 complete stereo frames.
- Updated the KOS backend lifecycle test to assert the named byte-size contract rather than re-deriving the value from a frame count.

## Validation

The prior milestone 4.57 host suite passed 44/44 tests and the KallistiOS SH-4 build completed successfully. This milestone's source and regression assertion have been committed; a fresh CI run for these commits has not yet been observed, so no new test pass is claimed here.

## Scope and limitations

This is contract clarity and regression hardening, not a new audio runtime. The AICA application remains a diagnostic tone test. No PS1 BIOS/game boot, Flycast audition, physical Dreamcast playback, or runnable CDI image is claimed.
