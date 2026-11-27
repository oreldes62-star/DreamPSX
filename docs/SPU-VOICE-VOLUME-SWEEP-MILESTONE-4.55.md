# SPU per-voice volume control milestone 4.55

## Scope

- Decode direct left/right voice volume as signed 15-bit register data doubled to the SPU's signed 16-bit current-volume range.
- Keep programmed volume registers separate from the current per-channel volume level.
- Add independent left/right sweep accumulators and active flags. The sweep rate uses the SPU's 0x8000 threshold accumulator, including the frozen all-ones rate encoding, sign/phase selection, direction-specific saturation, and exponential slow-down/scaling.
- Tick sweep state once per active voice per mixed output sample, before applying voice gain to dry and reverb-send paths.

## Regression coverage

- Positive, negative, and near-zero direct-volume decoding.
- Direct register writes update the current left/right level independently.
- Linear positive sweep increments by the expected step.
- Shift 12 requires two sample clocks per step.
- Negative-phase sweep moves in the negative direction.
- Exponential decrease scales its step by the current level.
- Sweeps stop at their direction-specific endpoint; the all-ones rate remains frozen.
- Invalid SPU and voice arguments are rejected.

## Validation and limitations

The rate accumulator and signed-phase rules were cross-checked against the published PSX-SPX description and the current ares PS1 SPU VolumeSweep implementation. Fine-grained hardware timing, register readback of current volume, and sweep restart edge cases still require comparison against physical-console traces. Host CMake build and all 44 CTest cases passed on the milestone 4.55 head. The separate KallistiOS SH-4 cross-build is still running; no Dreamcast ELF result is claimed for this milestone yet. No Flycast or physical Dreamcast execution is claimed.
