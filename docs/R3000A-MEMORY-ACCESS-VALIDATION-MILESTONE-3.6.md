# R3000A memory access validation — milestone 3.6

## Scope

This milestone strengthens source-level regression coverage for CPU memory
access faults after the LWL/LWR implementation in milestone 3.5.

## Added coverage

- Misaligned `LH` raises Address Error Load and preserves the effective
  virtual address in COP0 BadVAddr.
- Misaligned `SH` raises Address Error Store and preserves BadVAddr.
- Misaligned `SW` raises Address Error Store rather than a generic bus fault.
- An aligned `SW` to an unmapped address raises Bus Data and records the
  effective address in BadVAddr.
- The cases are exercised through `psx_machine_step()`, so the tests cover
  CPU result classification and machine-level COP0 exception dispatch.

## Existing behavior reviewed

- `LB/LBU` and `SB` are byte-addressable and do not require alignment.
- `LH/LHU/SH` require 2-byte alignment.
- `LW/SW` require 4-byte alignment.
- `LWL/LWR` use an aligned bus word read while retaining the original
  effective address for fault reporting.
- Signed byte and halfword loads extend the sign; unsigned variants zero-extend.
- Memory multi-byte writes preflight the complete span before modifying bytes.

## Validation status

Regression test source has been added to `tests/test_machine.c`. The project
is still not a complete buildable checkout, and these tests have **not** been
compiled or executed in this environment. No Dreamcast/KallistiOS build, CDI
image, or playable game is claimed by this milestone.

## Next step

Add direct execution tests for byte/halfword signedness and byte-lane store
behavior, then establish a reproducible host-side build and test command.
