# GTE command timing and CPU interlock — milestone 3.81

## Scope

DreamPSX now models command occupancy for the implemented R3000A GTE command set. A command computes against a private copy of the register file, while architectural data/control registers retain their previous values until the command's remaining clocks expire. The CPU waits before the next COP2 instruction that would access the GTE; independent CPU instructions continue while the GTE works.

## Latencies

The table uses the PSX-SPX command clock counts (the Sony Psy-Q reference lists RTPS/RTPT one clock lower):

| Command | Cycles |
|---|---:|
| RTPS | 15 |
| RTPT | 23 |
| NCLIP | 8 |
| OP | 6 |
| DPCS, INTPL, MVMVA | 8 |
| NCDS | 19 |
| NCDT | 44 |
| CDP | 13 |
| NCCS | 17 |
| CC | 11 |
| NCS | 14 |
| NCT | 30 |
| SQR | 5 |
| DCPL | 8 |
| DPCT | 17 |
| AVSZ3 | 5 |
| AVSZ4 | 6 |
| GPF, GPL | 5 |
| NCCT | 39 |

References:
- https://psx-spx.consoledev.net/ps1/cpu/gte/geometrytransformationenginegte/
- https://psx.arthus.net/sdk/Psy-Q/DOCS/Devrefs/Inlinref.pdf

## Implementation

- `PsxGte` now stores a pending data/control register image and a remaining busy-clock count.
- `psx_gte_issue()` computes into a temporary GTE state and queues the result.
- `psx_gte_advance_cycles()` publishes the result only when the countdown completes.
- `psx_gte_command_cycles()` centralizes the command latency table.
- COP2 instructions and LWC2/SWC2 data transfers interlock against a busy GTE. Ordinary CPU instructions consume one GTE clock while the unit is working.
- The existing `psx_gte_execute()` remains an immediate helper for isolated arithmetic tests; CPU execution uses the timed issue path.

## Validation added

- Direct GTE test confirms NCLIP's MAC0 result remains hidden during the seven post-issue clocks and appears at completion.
- CPU/machine tests confirm MFC2, LWC2 and SWC2 wait for the remaining seven clocks; the tests check completed results and the MFC2 load delay.
- Latency assertions cover RTPS, RTPT, NCLIP and NCDT.

These tests have been added to the repository but have not been compiled or run in this environment. No claim of passing CI or Dreamcast runtime compatibility is made.

## Follow-up correction\n\nThe initial CPU interlock covered COP2 opcodes but not the dedicated LWC2/SWC2 opcodes. Milestone follow-up commit now includes both memory-transfer forms and adds CPU/machine regression tests for them.\n\n## Remaining timing work

This is a first occupancy/interlock model, not a cycle-accurate GTE pipeline. Per-register read/write hazards, internal pipeline overlap, the exact RTPS/RTPT Sony-versus-PSX-SPX cycle convention, and interactions with interrupts/DMA still need dedicated validation. The CPU's general instruction cost remains a one-cycle baseline except for modeled HI/LO and GTE waits.
