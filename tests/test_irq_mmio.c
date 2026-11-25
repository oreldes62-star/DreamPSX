#include "dreampsx/irq_mmio.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    PsxIrqController irq;
    uint8_t b;
    uint16_t h;
    uint32_t w;

    psx_irq_reset(&irq);
    psx_irq_set_line(&irq, 0, true);
    psx_irq_set_line(&irq, 8, true);

    /* Byte reads expose the little-endian low/high lanes. */
    assert(psx_irq_mmio_read8(&irq, PSX_IRQ_STAT_ADDR, &b) && b == 1u);
    assert(psx_irq_mmio_read8(&irq, PSX_IRQ_STAT_ADDR + 1u, &b) && b == 1u);
    assert(psx_irq_mmio_read16(&irq, PSX_IRQ_STAT_ADDR, &h) && h == 0x0101u);
    assert(psx_irq_mmio_read32(&irq, PSX_IRQ_STAT_ADDR, &w) && w == 0x0101u);

    /* A byte write to the low I_STAT lane clears only that lane. */
    assert(psx_irq_mmio_write8(&irq, PSX_IRQ_STAT_ADDR, 0u));
    assert(psx_irq_read_stat(&irq) == 0x0100u);

    /* I_MASK byte writes preserve the other byte. */
    assert(psx_irq_mmio_write8(&irq, PSX_IRQ_MASK_ADDR + 1u, 1u));
    assert(psx_irq_mmio_read16(&irq, PSX_IRQ_MASK_ADDR, &h) && h == 0x0100u);

    /* Word writes use the low halfword; upper half is ignored. */
    assert(psx_irq_mmio_write32(&irq, PSX_IRQ_MASK_ADDR, 0xffff0001u));
    assert(psx_irq_read_mask(&irq) == 1u);

    /* Reject misaligned and unmapped accesses. */
    assert(!psx_irq_mmio_read16(&irq, PSX_IRQ_STAT_ADDR + 1u, &h));
    assert(!psx_irq_mmio_write32(&irq, PSX_IRQ_MASK_ADDR + 2u, 0u));
    assert(!psx_irq_mmio_read8(&irq, 0x1f801078u, &b));

    puts("DreamPSX IRQ MMIO tests: OK");
    return 0;
}
