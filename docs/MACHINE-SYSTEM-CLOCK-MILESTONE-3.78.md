# Shared system-cycle domain and Timer2 divider — milestone 3.78

## Changes

- Added `psx_machine_advance_system_cycles(machine, cycles)` as the common system-clock input for the root counters.
- Timer0, Timer1 and Timer2 are offered system-clock ticks; each counter's selected-source gate decides whether it advances.
- Timer2's divide-by-eight source now uses a persistent 0..7 phase remainder in `PsxMachine`. Partial intervals accumulate across calls, and a tick is emitted whenever eight system cycles have elapsed.
- Reset clears the divider phase.
- Integration tests cover split intervals (3 + 4 + 1 cycles), Timer2 increment and IRQ source 6.

## Integration boundary

The CPU currently executes one instruction per `psx_machine_step` call but does not yet expose reliable per-instruction cycle costs. Therefore this API is deliberately explicit: a future CPU timing layer or frontend scheduler must pass elapsed system cycles. This milestone does not claim cycle-accurate CPU/GPU synchronization, nor does it automatically advance time inside `psx_machine_step`.

Timer0 dot clock still requires GPU dot events; Timer1 HBlank remains the line-level approximation from milestone 3.77. Timer synchronization modes and sub-scanline timing remain future work.

## Validation

Tests were extended and registered in CMake. They have not been executed in this development environment; no build or runtime success is claimed.
