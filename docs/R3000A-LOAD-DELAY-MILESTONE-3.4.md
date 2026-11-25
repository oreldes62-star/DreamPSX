# R3000A Load Delay Slot — Milestone 3.4

## Scope

This milestone adds the one-instruction delayed write-back behavior for the
implemented R3000A integer memory-load instructions:

- LB, LBU
- LH, LHU
- LW

The CPU context stores one pending destination register and value. Source
register operands are read before the pending value is committed, so the
instruction immediately after a load observes the old value. The pending
value is then made visible after that instruction retires.

## Write conflicts and consecutive loads

- If the following instruction writes the same GPR with a non-load result,
  that newer instruction's result takes precedence.
- If the following instruction is another load, the older load is committed
  and the newer load is queued for the next instruction.
- Register zero never receives a pending load.
- JAL and JALR link-register writes participate in the normal write-back
  ordering rather than bypassing it.

## Exception boundary

The machine retires a pending load before transferring control to a synchronous
exception handler. The faulting instruction is not retired. The same retirement
path is used by machine-level interrupt/exception entry.

## Tests added (source)

The CPU execution test source now covers:
- old value observed by the immediate consumer;
- loaded value visible to the next instruction;
- same-register ALU overwrite;
- unrelated instruction retirement;
- consecutive loads to different destinations.

The machine test source covers retirement of a pending load before SYSCALL
exception entry.

## Limitations

This is an incremental R3000A model, not a cycle-accurate pipeline. LWL/LWR
are not implemented here, and their merge/load-delay interactions remain
future work. The repository is not yet a complete buildable checkout.
The added tests have been committed as source but have not been compiled or run
in a complete project build in this environment.
