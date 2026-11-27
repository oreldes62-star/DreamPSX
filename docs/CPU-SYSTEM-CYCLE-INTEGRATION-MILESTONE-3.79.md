# CPU system-cycle integration — milestone 3.79

## Scope

This milestone connects the existing CPU instruction-step API to the shared machine system-clock domain introduced in milestone 3.78.

## Changes

- `PsxCpu.last_step_cycles` records the baseline cycle estimate for the most recent attempted instruction.
- `psx_cpu_last_step_cycles()` exposes that value without exposing internal CPU state assumptions to callers.
- The current estimate is deliberately conservative: one system cycle per attempted instruction, including attempts that reach a synchronous CPU exception result after the CPU and bus have been accepted.
- `psx_machine_step()` forwards the recorded cycles to `psx_machine_advance_system_cycles()` after the CPU step.
- Timer0/Timer1 system-clock selections and Timer2's divide-by-eight source now receive time during normal machine instruction stepping.
- Timer IRQs raised by that advancement are refreshed into COP0 Cause and are sampled at the next instruction boundary.

## Validation added

The timer IRQ integration test now places an ADDIU instruction in RAM, configures Timer0 to interrupt at its first system tick, executes one machine step, and checks the register result, one-cycle baseline, timer count, IRQ controller latch and COP0 IP2 reflection.

## Explicit limitations

This is not cycle-accurate R3000A timing. It does not model instruction-dependent latency, multiply/divide latency, GTE command latency, memory wait states, bus contention, cache effects, branch penalties or DMA stalls. The one-cycle value is a temporary baseline so the machine's shared clock is no longer disconnected from CPU execution. Dot-clock Timer0 pulses and physical HBlank phase remain separate scheduler work.

The new test has been added but has not been compiled or executed in this development session. No Dreamcast binary or CDI image is produced by this milestone.
