# DreamPSX — COP2 branch research (BC2F / BC2T)

## Confirmed instruction encoding

The PlayStation CPU opcode reference lists the COP2 branch group as `rs = 0x08`:

- `rt = 0`: BC2F — branch when the COP2 condition flag is false.
- `rt = 1`: BC2T — branch when the COP2 condition flag is true.
- The signed 16-bit immediate is a word displacement relative to `PC + 4`.
- As with MIPS branches, the instruction in the delay slot executes whether or not the branch is taken.

Reference: https://psx-spx.consoledev.net/cpuspecifications/ (CPU Opcode Encoding; CPU Coprocessor Opcodes; CPU Jump Opcodes).

## COP0 gate

The PSX CPU specification identifies Status bit 30 as CU2. If the required coprocessor is not enabled, Cause.ExcCode is 0x0B (Coprocessor Unusable), and Cause.CE identifies the coprocessor number. This is handled by the preceding CU2 milestone.

## Implementation caution

Do not assume that the GTE FLAG register's summary bit can be used as the COP2 branch condition without verifying the PS1 hardware behavior. The generic MIPS definition only says that BC2F/BC2T test the coprocessor condition flag; it does not, by itself, prove how the PS1 GTE drives that signal.

Before implementing the branch handler, verify the PS1-specific condition source and how it is updated by GTE commands. Then add tests for both taken and not-taken branches, the delay slot, negative and positive displacements, and CU2-disabled exceptions.

## Status

Research note only. No BC2F/BC2T execution handler or new automated test is claimed by this document. A verified implementation still requires editing and running the source tree from the user's current archive.
