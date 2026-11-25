#include "dreampsx/cop0.h"

uint32_t psx_cop0_exception_entry(uint32_t *status, uint32_t *cause,
                                 uint32_t *epc, PsxExceptionCode code,
                                 uint32_t current_pc, bool in_delay_slot,
                                 uint32_t branch_pc)
{
    uint32_t vector;
    if (!status || !cause || !epc) return 0u;

    *epc = in_delay_slot ? branch_pc : current_pc;
    *cause &= ~(PSX_COP0_CAUSE_EXCCODE | PSX_COP0_CAUSE_BD);
    *cause |= ((uint32_t)code << PSX_COP0_CAUSE_EXCCODE_SHIFT) &
              PSX_COP0_CAUSE_EXCCODE;
    if (in_delay_slot) *cause |= PSX_COP0_CAUSE_BD;

    /* R3000A pushes the low six mode/interrupt-enable bits left by two. */
    *status = (*status & ~PSX_COP0_STATUS_EXCEPTION_STACK) |
              ((*status << 2) & PSX_COP0_STATUS_EXCEPTION_STACK);

    vector = (*status & PSX_COP0_STATUS_BEV) ? 0xbfc00180u : 0x80000080u;
    return vector;
}
