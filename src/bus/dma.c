#include "dreampsx/dma.h"

void psx_dma_reset(PsxDmaController *dma)
{
    if (!dma) return;
    for (unsigned i = 0u; i < PSX_DMA_CHANNEL_COUNT; ++i) {
        dma->channel[i].madr = 0u;
        dma->channel[i].bcr = 0u;
        dma->channel[i].chcr = 0u;
    }
    dma->dpcr = 0x07654321u;
    dma->dicr = 0u;
}

bool psx_dma_init(PsxDmaController *dma)
{
    if (!dma) return false;
    dma->initialized = true;
    psx_dma_reset(dma);
    return true;
}

bool psx_dma_read32(const PsxDmaController *dma, uint32_t address,
                    uint32_t *value)
{
    uint32_t offset;
    unsigned channel;
    unsigned reg;

    if (!dma || !dma->initialized || !value ||
        address < PSX_DMA_BASE_ADDR || address > PSX_DMA_DICR_ADDR ||
        (address & 3u) != 0u)
        return false;

    offset = address - PSX_DMA_BASE_ADDR;
    if (offset < 0x70u) {
        channel = offset / 0x10u;
        reg = (offset & 0x0fu) / 4u;
        if (channel >= PSX_DMA_CHANNEL_COUNT) return false;
        if (reg == 0u) *value = dma->channel[channel].madr;
        else if (reg == 1u) *value = dma->channel[channel].bcr;
        else if (reg == 2u) *value = dma->channel[channel].chcr;
        else return false;
        return true;
    }
    if (address == PSX_DMA_DPCR_ADDR) {
        *value = dma->dpcr;
        return true;
    }
    if (address == PSX_DMA_DICR_ADDR) {
        const uint32_t channel_flags = (dma->dicr >> 24u) & 0x7fu;
        const uint32_t channel_enables = (dma->dicr >> 16u) & 0x7fu;
        const bool master_irq =
            (dma->dicr & (1u << 15u)) != 0u ||
            ((dma->dicr & (1u << 23u)) != 0u &&
             (channel_flags & channel_enables) != 0u);
        *value = dma->dicr & 0x7fffffffu;
        if (master_irq) *value |= 1u << 31u;
        return true;
    }
    return false;
}

bool psx_dma_write32(PsxDmaController *dma, uint32_t address,
                     uint32_t value)
{
    uint32_t offset;
    unsigned channel;
    unsigned reg;

    if (!dma || !dma->initialized ||
        address < PSX_DMA_BASE_ADDR || address > PSX_DMA_DICR_ADDR ||
        (address & 3u) != 0u)
        return false;

    offset = address - PSX_DMA_BASE_ADDR;
    if (offset < 0x70u) {
        channel = offset / 0x10u;
        reg = (offset & 0x0fu) / 4u;
        if (channel >= PSX_DMA_CHANNEL_COUNT) return false;
        if (reg == 0u) dma->channel[channel].madr = value & 0x00fffffcu;
        else if (reg == 1u) dma->channel[channel].bcr = value;
        else if (reg == 2u) dma->channel[channel].chcr = value;
        else return false;
        return true;
    }
    if (address == PSX_DMA_DPCR_ADDR) {
        dma->dpcr = value & 0x0fffffffu;
        return true;
    }
    if (address == PSX_DMA_DICR_ADDR) {
        /* Control flags are writable; the low seven channel flags and the
         * master IRQ flag are write-one-to-clear status bits. */
        const uint32_t status_mask = 0x7f000000u;
        dma->dicr = (dma->dicr & status_mask & ~value) |
                    (value & 0x00ffffffu);
        return true;
    }
    return false;
}
