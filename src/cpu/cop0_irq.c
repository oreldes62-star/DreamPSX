#include "dreampsx/cop0_irq.h"

uint32_t psx_cop0_cause_with_irq(uint32_t cause, bool irq_output)
{
    if (irq_output) return cause | PSX_COP0_CAUSE_IP2;
    return cause & ~PSX_COP0_CAUSE_IP2;
}

uint32_t psx_cop0_cause_with_irq_controller(uint32_t cause,
                                            const PsxIrqController *irq)
{
    return psx_cop0_cause_with_irq(cause, psx_irq_output(irq));
}

bool psx_cop0_interrupt_eligible(uint32_t status, uint32_t cause)
{
    if ((status & PSX_COP0_STATUS_IEC) == 0u) return false;
    return (status & cause & PSX_COP0_STATUS_IM & PSX_COP0_CAUSE_IP) != 0u;
}
