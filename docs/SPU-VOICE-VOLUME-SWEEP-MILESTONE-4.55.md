# SPU per-voice volume control milestone 4.55

## Scope

- Decode direct left/right voice volume as signed 15-bit register data doubled to the SPU's signed 16-bit current-volume range.
- Keep programmed volume registers separate from the current per-channel volume level.
- Add an independent sample-clocked sweep state for left and right channels, including direction, phase, shift, step, and exponential rate behavior.
- Tick sweep state once per active voice per mixed output sample, before applying voice gain to dry and reverb-send paths.

## Regression coverage

- Positive, negative, and near-zero direct-volume decoding.
- Direct register writes update the current left/right level independently.
- Linear positive sweep increments by the expected step.
- Negative-phase sweep moves in the negative direction.
- Invalid SPU and voice arguments are rejected.

## Validation and limitations

This is a software model based on the published PSX-SPX register layout and sweep-rate description. The phase/direction arithmetic is implemented deterministically, but has not been validated against the new physical-console VOLXL traces in pcsx-redux. Fine-grained hardware timing, sweep restart behavior, and negative-phase edge cases remain candidates for hardware comparison. Host and KallistiOS CI results will be recorded after this commit; no Flycast or physical Dreamcast execution is claimed.
