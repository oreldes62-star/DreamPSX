# DreamPSX milestone 1.9 — COP0 interrupt eligibility helper

## Added

- `include/dreampsx/cop0_irq.h`
- `src/cpu/cop0_irq.c`
- `tests/test_cop0_irq.c`

## Behavior

- Mirrors the external IRQ-controller output into Cause.IP2 (bit 10), preserving every unrelated Cause bit.
- Checks the MIPS global interrupt enable bit Status.IEc.
- Checks matching pending and enabled interrupt lines using Status.IM and Cause.IP.
- Covers hardware IP2 and software interrupt lines in unit-test source.

## Important integration boundary

This helper does not take an exception, modify EPC, set Cause.BD, or clear I_STAT. Those operations belong in the CPU execution/exception path. The main CPU source and full build system are not currently present in this GitHub repository, so this module is not yet wired into the live CPU and its tests have not been run in a project build.

## Integration sequence

1. Bus routes I_STAT/I_MASK accesses through the IRQ MMIO adapter.
2. Machine state owns one IRQ controller.
3. CPU boundary updates Cause.IP2 with `psx_cop0_cause_with_irq()`.
4. CPU checks `psx_cop0_interrupt_eligible()` before normal instruction dispatch, while preserving delay-slot exception rules.
5. CPU exception entry records EPC/BD and enters the normal interrupt vector without acknowledging I_STAT.
