# DreamPSX milestone 2.5 — R3000A CPU context and BIOS fetch

## Delivered

- Added the first CPU context structure with 32 general-purpose registers, HI, LO, PC and next-PC state.
- Reset initializes the CPU at the PlayStation BIOS reset vector 0xBFC00000 and next-PC at 0xBFC00004.
- Register zero reads as zero and ignores writes; invalid register indices are safely rejected.
- Added instruction fetch through the machine CPU bus, so reset-vector fetches resolve through the BIOS mapping.
- PsxMachine now owns the CPU context and resets it with the rest of the machine.
- Added a test source that places a known little-endian word at the BIOS reset vector and checks fetch and register-zero behavior.

## Scope

This is CPU state and instruction-fetch plumbing, not an executing R3000A interpreter. There is no decode, ALU, branch-delay handling, exception entry, COP0 register instruction path or cycle scheduler yet. The repository still lacks a complete build system; the test source is committed but not reported as executed.

## Next

Implement a small, testable instruction subset (for example SPECIAL arithmetic and immediate loads) with correct PC/next-PC and delay-slot semantics, then grow toward BIOS boot execution.
