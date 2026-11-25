# Automatic hardware interrupt dispatch — milestone 3.1

## Implemented

- Machine stepping now refreshes the external IRQ controller output into COP0 Cause.IP2 before each instruction boundary.
- When Status.IEC, Status.IM and Cause.IP indicate an eligible interrupt, the machine enters the general interrupt exception automatically.
- Exception entry records EPC, sets Cause.ExcCode to Interrupt, applies the R3000A status-stack shift and redirects the CPU to the BEV-selected vector.
- Interrupt dispatch is deferred while a branch delay-slot instruction is pending, so the delay-slot instruction is allowed to complete before the interrupt is taken.
- Added machine test source covering automatic IRQ delivery, EPC, exception code, vector selection and interrupt masking after exception entry.

## Scope and limitations

- The current machine-step API still returns a boolean and does not distinguish a dispatched interrupt from a successfully executed instruction.
- Synchronous CPU faults (reserved instruction, address error, bus error, overflow and COP0 privilege violation) are not yet converted automatically into their corresponding exception codes.
- Interrupt priority/arbitration and device-specific acknowledgement remain the responsibility of the IRQ controller and emulated device logic.

## Validation status

The test source is committed but has not been compiled or executed. The repository is still not a complete buildable checkout, and no Dreamcast/KallistiOS build or CDI image has been produced.
