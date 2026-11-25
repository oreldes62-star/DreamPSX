#include "dreampsx/cop0_irq.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    PsxIrqController irq;
    psx_irq_reset(&irq);

    /* A pending source alone must not assert CPU IP2 while masked. */
    psx_irq_set_line(&irq, 0u, true);
    uint32_t cause = psx_cop0_cause_with_irq_controller(0x80000001u, &irq);
    assert((cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert((cause & 0x80000001u) == 0x80000001u);

    /* Enabling its I_MASK bit raises the external IRQ output and IP2. */
    psx_irq_write_mask(&irq, 1u);
    cause = psx_cop0_cause_with_irq_controller(cause, &irq);
    assert((cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_cop0_interrupt_eligible(PSX_COP0_STATUS_IEC |
                                       PSX_COP0_CAUSE_IP2, cause));

    /* Clearing I_STAT drops the controller output and CPU IP2. */
    psx_irq_write_stat(&irq, (uint16_t)~1u);
    cause = psx_cop0_cause_with_irq_controller(cause, &irq);
    assert((cause & PSX_COP0_CAUSE_IP2) == 0u);

    puts("DreamPSX IRQ-to-COP0 integration tests: OK");
    return 0;
}
