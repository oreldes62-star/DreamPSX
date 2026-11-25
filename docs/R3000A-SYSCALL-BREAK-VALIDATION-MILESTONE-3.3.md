# R3000A SYSCALL, BREAK and encoding validation — milestone 3.3

## Implemented

- Decoded SPECIAL funct 0x0C as SYSCALL and funct 0x0D as BREAK.
- Added distinct CPU step results for both instructions.
- Machine stepping maps them to COP0 exception codes 8 (Syscall) and 9 (Breakpoint).
- Exception entry preserves EPC and Cause.BD semantics when either instruction is executed in a branch delay slot.
- Added validation of fixed fields for the implemented SPECIAL encodings (shift, jump-register, HI/LO transfer, multiply/divide and register arithmetic/logical instructions).
- Tightened COP0 transfer encodings: MFC0/MTC0 require reserved low fields to be zero, and RFE requires its exact low-field encoding.
- Updated the existing user-mode COP0 test to expect a dispatched Coprocessor Unusable exception rather than a raw step failure.
- Added test source for SYSCALL, BREAK and malformed SPECIAL encoding.

## Scope and limitations

- This is incremental validation, not a complete MIPS-I decoder or exhaustive reserved-bit audit.
- Other R3000A instructions and PS1-specific coprocessor behavior remain incomplete.
- Exception return flow, syscall services and BIOS service behavior are not implemented by merely decoding SYSCALL; the BIOS exception handler must still execute.

## Validation status

Test source has been updated and committed but has not been compiled or run. The GitHub repository is not yet a complete buildable checkout; no KallistiOS build, CDI image or playable PS1 game has been produced.
