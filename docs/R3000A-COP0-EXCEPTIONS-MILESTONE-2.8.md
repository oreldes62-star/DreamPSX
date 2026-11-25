# R3000A COP0 exception entry — milestone 2.8

## Implemented

- Added the COP0 exception-code definitions used by the R3000A.
- Added Status.BEV, Cause.BD and Cause.ExcCode masks.
- Implemented exception entry: EPC capture, Cause.ExcCode update, delay-slot BD flag, and the six-bit Status mode/interrupt stack shift.
- Selects the bootstrap exception vector 0xBFC00180 when BEV is set, otherwise 0x80000080.
- Added COP0 EPC and BadVAddr storage to PsxMachine.
- Added `psx_machine_raise_exception` to update COP0 state and redirect the CPU to the selected vector.
- Added tests for ordinary and delay-slot exception entry, including BadVAddr and preservation of unrelated Cause bits.

## Validation status

The test source is committed but has not been compiled or run. The repository is not yet a complete buildable checkout. Exception entry is exposed as a machine-level operation; instruction decode, bus faults and interrupt dispatch do not yet automatically invoke it.

## Next work

- Route ADDI overflow, address errors and reserved instructions through this exception path.
- Add MFC0/MTC0 instruction execution and COP0 register access rules.
- Track delay-slot state precisely in the CPU.
- Implement interrupt checks at instruction boundaries and validate the BIOS boot path.
- Establish a reproducible KallistiOS build before claiming a Dreamcast binary or CDI.
