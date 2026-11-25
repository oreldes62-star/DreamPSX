# DreamPSX 1.3 — COP2 branch instructions

## Implemented

- Decode COP2 branch group (`rs=0x08`).
- `BC2F` (`rt=0`): branch taken, following the PS1-specific PSX-SPX observation that the GTE condition line appears permanently false.
- `BC2T` (`rt=1`): branch not taken.
- Signed 16-bit word displacement relative to `PC+4`.
- The instruction after the branch executes as the delay slot.
- Invalid `rt` values in this branch group raise Reserved Instruction.

## Tests

CPU tests cover BC2F taken path, target and delay slot, plus BC2T not-taken and sequential execution.

## Evidence and caveat

PSX-SPX's GTE overview describes `jt cop2flg` as never taken and `jf cop2flg` as always taken, noting that the flag appears to be always false:
https://psx-spx.consoledev.net/ps1/cpu/gte/geometrytransformationenginegte/

This is a documented PS1 observation, not yet validated by DreamPSX against physical hardware. Keep it explicitly marked as such.

## Not implemented by this milestone

- GTE cycle timing / stalls.
- Complete CPU exception and branch-delay corner cases.
- Dreamcast/KallistiOS target build.
- CDI image or playable game.
