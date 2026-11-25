#include "dreampsx/cop0_irq.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    const uint32_t unrelated = (1u << 31) | (1u << 6) | 0x123u;
    uint32_t cause = psx_cop0_cause_with_irq(unrelated, true);
    assert((cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert((cause & ~PSX_COP0_CAUSE_IP2) == unrelated);

    cause = psx_cop0_cause_with_irq(cause, false);
    assert((cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert((cause & ~PSX_COP0_CAUSE_IP2) == unrelated);

    /* No global enable: masked pending interrupt is not eligible. */
    assert(!psx_cop0_interrupt_eligible(PSX_COP0_STATUS_IM,
                                       PSX_COP0_CAUSE_IP2));

    /* IEc set but IP2 masked: not eligible. */
    assert(!psx_cop0_interrupt_eligible(PSX_COP0_STATUS_IEC,
                                       PSX_COP0_CAUSE_IP2));

    /* IEc and matching IM2/IP2 set: eligible. */
    assert(psx_cop0_interrupt_eligible(PSX_COP0_STATUS_IEC |
                                      PSX_COP0_CAUSE_IP2,
                                      PSX_COP0_CAUSE_IP2));

    /* An enabled software interrupt line is also recognized. */
    assert(psx_cop0_interrupt_eligible(PSX_COP0_STATUS_IEC | (1u << 8),
                                       (1u << 8)));

    /* Pending line masked in Status is ignored. */
    assert(!psx_cop0_interrupt_eligible(PSX_COP0_STATUS_IEC | (1u << 9),
                                        (1u << 8)));

    puts("DreamPSX COP0 IRQ tests: OK");
    return 0;
}
