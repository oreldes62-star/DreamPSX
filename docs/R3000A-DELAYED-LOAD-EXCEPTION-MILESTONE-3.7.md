# R3000A delayed-load exception boundary — milestone 3.7

## Scope

This milestone extends regression coverage for the interaction between the
R3000A one-instruction load delay and machine-level synchronous exceptions.

## Added regression case

- A valid `LW` queues a value in a GPR.
- The immediately following `LW` accesses an unmapped address and raises a
  Bus Data exception.
- Exception entry retires the older pending load before redirecting execution.
- The faulting load does not overwrite its destination register.
- The pending-load state is cleared, and COP0 records the faulting instruction
  PC and effective address.

This exercises the CPU's delayed write-back path together with
`psx_machine_step()` and `psx_machine_raise_exception()`.

## Existing coverage retained

The machine tests also cover pending-load retirement before SYSCALL exception
entry, and the CPU tests cover ordinary delayed loads, consecutive loads,
same-register write conflicts, and adjacent LWL/LWR merging.

## Validation status

The regression test is committed as source. It has not been compiled or
executed in this environment. GitHub reports no configured status checks for
the commit. No full Dreamcast build or CDI image is claimed.

## Next step

Add a matching interrupt-boundary regression, then create a minimal host build
configuration so the CPU, bus, memory and machine tests can be compiled and run
reproducibly.
