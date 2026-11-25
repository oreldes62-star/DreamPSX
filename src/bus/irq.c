#include "dreampsx/irq.h"

void psx_irq_reset(PsxIrqController *irq)
{
    if (!irq) return;
    irq->pending = 0;
    irq->mask = 0;
    irq->source_level = 0;
}

void psx_irq_set_line(PsxIrqController *irq, unsigned source, bool asserted)
{
    if (!irq || source >= PSX_IRQ_SOURCE_COUNT) return;
    const uint16_t bit = (uint16_t)(1u << source);
    const bool was_asserted = (irq->source_level & bit) != 0;

    if (asserted) {
        irq->source_level |= bit;
        if (!was_asserted) irq->pending |= bit;
    } else {
        irq->source_level &= (uint16_t)~bit;
    }
}

uint16_t psx_irq_read_stat(const PsxIrqController *irq)
{
    return irq ? (uint16_t)(irq->pending & PSX_IRQ_VALID_MASK) : 0;
}

uint16_t psx_irq_read_mask(const PsxIrqController *irq)
{
    return irq ? (uint16_t)(irq->mask & PSX_IRQ_VALID_MASK) : 0;
}

void psx_irq_write_stat(PsxIrqController *irq, uint16_t value)
{
    if (!irq) return;
    /* PS1 I_STAT is write-zero-to-clear: ones preserve pending requests. */
    irq->pending &= (uint16_t)(value | (uint16_t)~PSX_IRQ_VALID_MASK);
    irq->pending &= PSX_IRQ_VALID_MASK;
}

void psx_irq_write_mask(PsxIrqController *irq, uint16_t value)
{
    if (!irq) return;
    irq->mask = (uint16_t)(value & PSX_IRQ_VALID_MASK);
}

bool psx_irq_output(const PsxIrqController *irq)
{
    return irq && ((irq->pending & irq->mask & PSX_IRQ_VALID_MASK) != 0);
}
