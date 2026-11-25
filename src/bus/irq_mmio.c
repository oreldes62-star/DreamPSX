#include "dreampsx/irq_mmio.h"

static bool is_stat(uint32_t address)
{
    return address >= PSX_IRQ_STAT_ADDR && address < PSX_IRQ_STAT_ADDR + 2u;
}

static bool is_mask(uint32_t address)
{
    return address >= PSX_IRQ_MASK_ADDR && address < PSX_IRQ_MASK_ADDR + 2u;
}

static uint16_t read_register(const PsxIrqController *irq, bool stat)
{
    return stat ? psx_irq_read_stat(irq) : psx_irq_read_mask(irq);
}

static void write_register(PsxIrqController *irq, bool stat, uint16_t value)
{
    if (stat) psx_irq_write_stat(irq, value);
    else psx_irq_write_mask(irq, value);
}

bool psx_irq_mmio_read8(const PsxIrqController *irq, uint32_t address, uint8_t *value)
{
    if (!irq || !value || (!is_stat(address) && !is_mask(address))) return false;
    const bool stat = is_stat(address);
    const unsigned shift = (unsigned)(address & 1u) * 8u;
    *value = (uint8_t)(read_register(irq, stat) >> shift);
    return true;
}

bool psx_irq_mmio_read16(const PsxIrqController *irq, uint32_t address, uint16_t *value)
{
    if (!irq || !value || (address & 1u) ||
        (address != PSX_IRQ_STAT_ADDR && address != PSX_IRQ_MASK_ADDR)) return false;
    *value = read_register(irq, address == PSX_IRQ_STAT_ADDR);
    return true;
}

bool psx_irq_mmio_read32(const PsxIrqController *irq, uint32_t address, uint32_t *value)
{
    if (!irq || !value || (address & 3u) ||
        (address != PSX_IRQ_STAT_ADDR && address != PSX_IRQ_MASK_ADDR)) return false;
    *value = read_register(irq, address == PSX_IRQ_STAT_ADDR);
    return true;
}

bool psx_irq_mmio_write8(PsxIrqController *irq, uint32_t address, uint8_t value)
{
    if (!irq || (!is_stat(address) && !is_mask(address))) return false;
    const bool stat = is_stat(address);
    const unsigned shift = (unsigned)(address & 1u) * 8u;
    const uint16_t old = read_register(irq, stat);
    const uint16_t lane_mask = (uint16_t)(0xffu << shift);
    const uint16_t merged = (uint16_t)((old & (uint16_t)~lane_mask) |
                                      ((uint16_t)value << shift));
    write_register(irq, stat, merged);
    return true;
}

bool psx_irq_mmio_write16(PsxIrqController *irq, uint32_t address, uint16_t value)
{
    if (!irq || (address & 1u) ||
        (address != PSX_IRQ_STAT_ADDR && address != PSX_IRQ_MASK_ADDR)) return false;
    write_register(irq, address == PSX_IRQ_STAT_ADDR, value);
    return true;
}

bool psx_irq_mmio_write32(PsxIrqController *irq, uint32_t address, uint32_t value)
{
    if (!irq || (address & 3u) ||
        (address != PSX_IRQ_STAT_ADDR && address != PSX_IRQ_MASK_ADDR)) return false;
    write_register(irq, address == PSX_IRQ_STAT_ADDR, (uint16_t)value);
    return true;
}
