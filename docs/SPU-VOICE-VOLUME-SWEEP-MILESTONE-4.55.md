# SPU voice and master volume control milestones 4.55–4.56

## Scope

- Decode direct left/right voice volume as signed 15-bit register data doubled to the SPU's signed 16-bit current-volume range.
- Keep programmed volume registers separate from the current per-channel volume level.
- Add independent left/right sweep accumulators and active flags. The sweep rate uses the SPU's 0x8000 threshold accumulator, including the frozen all-ones rate encoding, sign/phase selection, direction-specific saturation, and exponential slow-down/scaling.
- Tick voice sweep state once per active voice per mixed output sample, before applying voice gain to dry and reverb-send paths.

## Milestone 4.56 — master stereo volume

- Add independent left/right main-volume registers, current levels, sweep counters and active flags.
- Apply master left/right gain after the dry and wet paths are summed.
- Advance the main-volume sweep once per output sample, including silent samples, so its timebase does not depend on voice activity.
- Reset all master-volume sweep state with the SPU.

## Regression coverage

- Positive, negative, and near-zero direct-volume decoding.
- Direct register writes update the current left/right level independently.
- Linear positive sweep increments by the expected step.
- Shift 12 requires two sample clocks per step.
- Negative-phase sweep moves in the negative direction.
- Exponential decrease scales its step by the current level.
- Sweeps stop at their direction-specific endpoint; the all-ones rate remains frozen.
- Invalid SPU and voice arguments are rejected.
- Master left/right register writes decode independently and the master sweep advances once per output sample, including when no voice is active.

## Validation and limitations

The rate accumulator and signed-phase rules were cross-checked against the published PSX-SPX description and the current ares PS1 SPU VolumeSweep implementation. Fine-grained hardware timing, register readback of current volume, and sweep restart edge cases still require comparison against physical-console traces. Host CMake build and all 44 CTest cases passed after the sweep-accuracy refinements (GitHub Actions run 46075121122). The KallistiOS SH-4 cross-build and AICA smoke ELF verification passed (GitHub Actions run 46075121126; artifact `dreampsx-dreamcast-build`). No Flycast or physical Dreamcast execution is claimed. Milestone 4.56 master-volume integration is awaiting fresh CI validation.
