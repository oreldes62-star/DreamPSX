#ifndef DREAMPSX_COP0_IRQ_H
#define DREAMPSX_COP0_IRQ_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/irq.h"

/* MIPS-I COP0 Status/Cause interrupt fields used by the PS1 CPU. */
#define PSX_COP0_STATUS_IEC  (1u << 0)
#define PSX_COP0_STATUS_IM   0x0000ff00u
#define PSX_COP0_CAUSE_IP    0x0000ff00u
#define PSX_COP0_CAUSE_IP2   (1u << 10)

/* Reflect the external interrupt-controller output into Cause.IP2.
 * All unrelated Cause bits are preserved.
 */
uint32_t psx_cop0_cause_with_irq(uint32_t cause, bool irq_output);

/* Convenience bridge: derive Cause.IP2 directly from the IRQ controller. */
uint32_t psx_cop0_cause_with_irq_controller(uint32_t cause,
                                            const PsxIrqController *irq);

/* True when global interrupts are enabled and at least one pending
 * interrupt line is enabled by the corresponding Status.IM bit.
 * This checks eligibility only; exception entry/EPC/BD handling remains
 * the CPU core's responsibility.
 */
bool psx_cop0_interrupt_eligible(uint32_t status, uint32_t cause);

#endif
