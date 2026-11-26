# R3000A LWL/LWR — Milestone 3.5

## Implemented

- LWL (opcode 0x22) and LWR (opcode 0x26).
- Little-endian byte-lane merge semantics for all four address offsets.
- Memory is read as an aligned 32-bit word; the effective unaligned address
  selects the byte-lane merge.
- Both instructions use the existing delayed-load write-back path.
- MIPS I adjacent-pair exception: if an LWL/LWR immediately follows an
  LWL/LWR to the same target register, it merges against the preceding pending
  result rather than the stale GPR value.

## Source tests added

The CPU execution test source covers each offset for LWL and LWR, delayed
write-back, and an adjacent LWL/LWR pair forming an unaligned word.

## Limitations

This implementation targets the PS1's little-endian R3000A environment and
does not implement big-endian mode, virtual-memory translation, cache behavior,
or cycle-accurate bus timing. The tests have been added as source but have not
been compiled or run in a complete project build in this environment.
