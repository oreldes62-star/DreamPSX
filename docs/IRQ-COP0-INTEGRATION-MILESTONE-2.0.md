# DreamPSX milestone 2.0 — IRQ to COP0 bridge

## Added integration point

The COP0 helper now accepts the IRQ controller directly through
`psx_cop0_cause_with_irq_controller()`. It derives Cause.IP2 from
`psx_irq_output()`, so a masked pending request does not reach the CPU,
while a pending-and-enabled request asserts IP2.

A focused integration test covers:
- pending but masked source;
- enabling I_MASK and asserting Cause.IP2;
- Status.IEc + IM2 eligibility;
- clearing I_STAT and deasserting Cause.IP2;
- preservation of unrelated Cause bits.

## What this does not claim

This is a tested module-level bridge, not yet the live R3000A interrupt
exception path. The CPU must still sample eligibility at architecturally
correct instruction boundaries, handle delay-slot EPC/BD state, enter the
exception vector, and leave I_STAT acknowledgement to software/hardware
semantics rather than clearing it on exception entry.

The repository still lacks a complete unified CPU/bus build target and
Dreamcast CDI output. This milestone does not create a playable emulator.
