# R3000A COP0 instruction support — milestone 3.0

## Implemented

- Added machine-aware CPU stepping so the interpreter can access the live COP0 Status, Cause, EPC and BadVAddr fields.
- Implemented MFC0 reads for BadVAddr, Status, Cause, EPC and PRId.
- Implemented MTC0 writes for Status and EPC, and software interrupt pending bits IP0/IP1 in Cause.
- Kept BadVAddr and PRId read-only; hardware interrupt pending bits are preserved on Cause writes.
- Implemented RFE status-stack restoration.
- Rejects COP0 instructions in user mode and rejects unsupported COP0 register/operation encodings.
- Preserved the existing bus-only CPU-step API for non-COP0 tests and callers.
- Added machine-step tests for MTC0, MFC0, RFE and user-mode privilege rejection.

## Scope and limitations

The machine-step entry point refreshes the external IRQ mirror before executing an instruction. It does not yet automatically dispatch an eligible interrupt, classify CPU faults into exception codes, or recover from unsupported instructions. Those need a richer CPU step result than the current boolean API.

## Validation status

Tests are committed as source only and have not been compiled or executed. The GitHub repository remains an incomplete source checkout without a complete build system or KallistiOS target.
