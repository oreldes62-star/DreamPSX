# Synchronous CPU exception dispatch — milestone 3.2

## Implemented

- Added a detailed CPU step result enum while preserving the existing boolean step wrappers for compatibility.
- Classified instruction-fetch bus faults, data bus faults, reserved instructions, signed ADDI overflow, COP0 privilege violations and unpredictable control transfers in a delay slot.
- Captured the faulting virtual address for fetch and data-access failures.
- Added alignment checks for instruction fetches, LH/LHU/LW and SH/SW.
- Machine stepping now converts those results into COP0 exceptions: Bus Instruction, Bus Data, Address Error Load/Store, Reserved Instruction, Overflow and Coprocessor Unusable.
- Exception entry preserves delay-slot context: EPC points to the branch and Cause.BD is set when the fault occurs in its delay slot.
- Added test source for overflow, reserved instruction, data bus error, misaligned load, COP0 privilege violation and overflow in a delay slot.

## Scope and limitations

- Address translation and bus faults remain simplified; the PS1 has no general-purpose TLB, and this milestone does not model all hardware-specific bus error distinctions.
- Unpredictable branch-in-delay-slot behavior is currently surfaced as Reserved Instruction for deterministic emulator behavior; real hardware behavior is not guaranteed.
- Syscall and BREAK decoding, full instruction legality checks and more complete load/store semantics remain future work.

## Validation status

Tests are committed as source but have not been compiled or executed in this repository. The GitHub repository is not yet a complete buildable checkout, and there is still no KallistiOS build or CDI image.
