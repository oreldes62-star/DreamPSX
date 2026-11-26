# R3000A conditional trap instructions — milestone 3.18

## Scope

Implemented the MIPS-I conditional trap instructions used by the PlayStation R3000A core.

### SPECIAL

- TGE / TGEU
- TLT / TLTU
- TEQ / TNE

### REGIMM immediate

- TGEI / TGEIU
- TLTI / TLTIU
- TEQI / TNEI

Signed comparisons use the core's portable signed comparison helper. Unsigned comparisons operate on the 32-bit register values and the sign-extended immediate, as required by the instruction encoding.

A true condition returns a dedicated CPU trap result. The machine translates it to COP0 exception code 13 and preserves the normal EPC / branch-delay exception-entry path. A false condition retires as an ordinary instruction and does not create a branch delay slot.

## Validation

Host CI passed on commit `c9023576967a38a5ef0fc67da45d6e0e4da6720a`.

The machine tests cover:
- A true TEQ register trap and exception code 13 / EPC.
- A false TNEI condition that advances normally.
- A signed TLTI comparison using a negative immediate.

This is host-side validation only. It does not constitute a Dreamcast/KallistiOS build, BIOS boot, CDI image, or playable-game test.
