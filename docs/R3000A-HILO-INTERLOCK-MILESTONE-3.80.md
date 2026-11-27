# R3000A HI/LO execution interlocks — milestone 3.80

## Changes

- MULT and MULTU now start an asynchronous HI/LO result with operand-dependent latency: 6, 9 or 13 cycles.
- Signed MULT selects latency from the signed magnitude of rs; MULTU uses the unsigned rs ranges.
- DIV and DIVU now use a 36-cycle completion latency.
- Results are held in pending HI/LO state and become architecturally visible when the unit completes.
- MFHI, MFLO, MTHI, MTLO and a subsequent MULT/DIV operation interlock until the pending operation completes.
- Independent instructions continue while the HI/LO unit is busy; each such instruction consumes one baseline cycle and advances the pending unit.
- Interlock wait cycles are included in `psx_cpu_last_step_cycles()`, so the machine's system timers advance during stalls.

## Validation added

CPU execution tests cover a fast MULTU followed immediately by MFLO (6-cycle interlock), medium signed MULT (9-cycle interlock), and DIVU followed by MFLO (36-cycle interlock).

## Scope and limitations

The latency ranges are based on the PlayStation R3000A instruction timing reference and measured timing notes. This is a first HI/LO execution-unit model, not a complete pipeline simulator. It does not yet model multiply/divide cancellation semantics for MTHI/MTLO during an active operation, exact instruction overlap in all pipeline situations, GTE command busy timing, load-use stalls beyond the existing architectural load-delay behavior, cache misses, RAM/bus wait states, or bus contention. For safety, HI/LO writes currently wait for the pending operation rather than modeling hardware-specific partial-result cancellation.

The tests were added but not compiled or executed in this session. No Dreamcast build or CDI image is produced.
