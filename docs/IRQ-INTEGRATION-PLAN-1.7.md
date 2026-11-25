# DreamPSX milestone 1.7 — IRQ integration plan

## Goal

Connect the standalone interrupt controller from milestone 1.6 to the PS1 memory-mapped I/O bus and the CPU's COP0 interrupt state. This milestone is an integration specification; it does not claim that the integration code exists yet.

## Address decoding

The I/O bus must decode the two 16-bit interrupt registers:

- `0x1F801070`: I_STAT (pending interrupt flags)
- `0x1F801074`: I_MASK (interrupt enable mask)

The PS1 bus is 32-bit-addressed, while these registers are 16-bit wide. The bus layer must define and test the behavior of byte and halfword accesses, including writes to either half of an aligned word. Do not silently treat every access as a full 32-bit register write.

## CPU connection

At each CPU interrupt-recognition boundary:

1. Read the controller output: `psx_irq_output()`.
2. Reflect it into COP0 Cause.IP2 (bit 10), preserving unrelated Cause fields.
3. Apply COP0 Status interrupt enable (IEc) and interrupt mask (IM2) before taking an interrupt exception.
4. If accepted, raise the normal interrupt exception (ExcCode 0) with the correct EPC and delay-slot/BD handling.
5. Keep the external IRQ level reflected in Cause.IP2 after exception entry; exception entry must not acknowledge I_STAT.

The I_STAT acknowledgement belongs to software/device interaction, not to CPU exception entry.

## Required integration tests

- Pending but masked source: I_STAT set, Cause.IP2 set, no exception taken.
- Pending and unmasked source with Status.IEc enabled: interrupt exception taken.
- Status.IEc disabled: no exception taken.
- I_MASK cleared after request: Cause.IP2 remains asserted only if another enabled pending source remains.
- I_STAT acknowledged: output falls when no other enabled request remains.
- Exception entry does not clear I_STAT.
- Interrupt arriving during a branch delay slot: EPC and Cause.BD are correct.
- Byte, halfword and word accesses to both registers preserve unaffected lanes.

## Constraints

- Do not call the IRQ controller from individual device implementations through ad-hoc global state. The bus/device owner should own the controller and expose a narrow interface.
- Do not conflate the controller's masked output with the raw pending bits.
- Do not implement DMA or GPU IRQ generation until their device events can call `psx_irq_set_line()` with the correct source number.
- Preserve the standalone controller tests as a separate unit-test target.

## Acceptance criteria

Milestone 1.7 is complete only when the CPU, bus and IRQ controller are wired together in the source tree and the integration tests above run successfully. A documentation-only commit is not an implementation milestone.
