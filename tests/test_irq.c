#include "dreampsx/irq.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    PsxIrqController irq;
    psx_irq_reset(&irq);
    assert(psx_irq_read_stat(&irq) == 0);
    assert(!psx_irq_output(&irq));

    /* A low-to-high transition latches one pending request. */
    psx_irq_set_line(&irq, 0, true);
    assert(psx_irq_read_stat(&irq) == 1u);
    psx_irq_set_line(&irq, 0, true); /* held high: no new edge */
    assert(psx_irq_read_stat(&irq) == 1u);

    /* Mask gates the CPU-facing IRQ output, not the pending bit. */
    assert(!psx_irq_output(&irq));
    psx_irq_write_mask(&irq, 1u);
    assert(psx_irq_output(&irq));

    /* Writing one preserves bit 0; writing zero clears it. */
    psx_irq_write_stat(&irq, 1u);
    assert(psx_irq_read_stat(&irq) == 1u);
    psx_irq_write_stat(&irq, 0u);
    assert(psx_irq_read_stat(&irq) == 0u);
    assert(!psx_irq_output(&irq));

    /* A new edge after the source went low is captured. */
    psx_irq_set_line(&irq, 0, false);
    psx_irq_set_line(&irq, 0, true);
    assert(psx_irq_read_stat(&irq) == 1u);

    /* Independent sources remain independently pending. */
    psx_irq_set_line(&irq, 3, true);
    assert(psx_irq_read_stat(&irq) == 9u);
    psx_irq_write_stat(&irq, (uint16_t)~1u);
    assert(psx_irq_read_stat(&irq) == 8u);

    /* Unused bits are masked off. */
    psx_irq_write_mask(&irq, 0xffffu);
    assert(psx_irq_read_mask(&irq) == PSX_IRQ_VALID_MASK);
    puts("DreamPSX IRQ tests: OK");
    return 0;
}
