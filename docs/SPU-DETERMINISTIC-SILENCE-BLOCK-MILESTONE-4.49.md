# SPU deterministic silence block validation milestone 4.49

## Change

The SPU test suite now verifies that block rendering on an initialized but inactive SPU overwrites every requested interleaved stereo sample with zero. This protects the audio pipeline from leaking stale samples when voices have ended or no voices are active.

The test also checks invalid block-render arguments:
- a non-zero frame request with a null output buffer is rejected and reports zero frames written;
- a null frames-written result pointer is rejected.

## Why this matters

The machine audio scheduler and Dreamcast callback consume fixed-size PCM blocks. Explicit silence is part of the output contract: a quiet frame must be zero-filled, never left as uninitialized or previously used memory.

## Validation status

The regression test is committed. Host CI must complete for this commit. This does not yet establish hardware-accurate SPU timing, full reverb/noise support, XA/SPU mixing, or real Dreamcast audio playback.
