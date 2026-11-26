# R3000A SWL/SWR — Milestone 3.15

## Implemented

- SWL (opcode 0x2A) and SWR (opcode 0x2E).
- Little-endian partial-word stores for all four effective-address byte offsets.
- SWL writes byte lanes 0 through the effective offset, sourcing the most
  significant register bytes in order.
- SWR writes byte lanes from the effective offset through lane 3, sourcing
  register bytes starting at the least significant byte.
- Stores use CPU-bus byte writes for the selected lanes; the effective address
  may be unaligned and is retained for data-bus fault reporting.
- Existing aligned SW behavior is unchanged.

## Source tests added

The CPU execution test source checks SWL and SWR at all four byte offsets.
Each case starts with a known memory word and register value, then verifies
that selected byte lanes are replaced while untouched lanes are preserved.

## Validation

GitHub Actions host build and CTest passed on commit
`6359f4b4532058e967cdb521c6c3c187d19141d1`:

- CMake configure: passed
- Host build: passed
- CTest: 12/12 tests passed, including `test_cpu_execution`

This validates the host-side implementation and regression suite only. It is
not a Dreamcast/KallistiOS target build, BIOS boot, CDI image, or gameplay test.

## Limitations

The core remains a partial R3000A implementation. This milestone does not add
cycle-accurate bus timing, cache behavior, virtual-memory translation, or
complete MMIO device semantics.
