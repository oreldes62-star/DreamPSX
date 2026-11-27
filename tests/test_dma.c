#include "dreampsx/dma.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    PsxDmaController dma = {0};
    uint32_t value = 0u;

    assert(psx_dma_init(&dma));
    assert(psx_dma_read32(&dma, PSX_DMA_DPCR_ADDR, &value));
    assert(value == 0x07654321u);
    assert(psx_dma_read32(&dma, PSX_DMA_DICR_ADDR, &value));
    assert(value == 0u);

    /* Channel 2 is the GPU channel; each channel has MADR/BCR/CHCR. */
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x20u,
                           0x80123457u));
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x24u,
                           0x00030010u));
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x28u,
                           0x01000201u));
    assert(psx_dma_read32(&dma, PSX_DMA_BASE_ADDR + 0x20u, &value));
    assert(value == 0x00123454u); /* MADR is 24-bit and word-aligned. */
    assert(psx_dma_read32(&dma, PSX_DMA_BASE_ADDR + 0x24u, &value));
    assert(value == 0x00030010u);
    assert(psx_dma_read32(&dma, PSX_DMA_BASE_ADDR + 0x28u, &value));
    assert(value == 0x01000201u);

    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x0fffffffu));
    assert(psx_dma_read32(&dma, PSX_DMA_DPCR_ADDR, &value));
    assert(value == 0x0fffffffu);

    /* DICR control bits are writable; channel flags are W1C. */
    assert(psx_dma_write32(&dma, PSX_DMA_DICR_ADDR, 0x00ff0000u));
    assert(psx_dma_read32(&dma, PSX_DMA_DICR_ADDR, &value));
    assert(value == 0x00ff0000u);
    assert(psx_dma_write32(&dma, PSX_DMA_DICR_ADDR, 0x81000000u));
    assert(psx_dma_read32(&dma, PSX_DMA_DICR_ADDR, &value));
    assert(value == 0x00ff0000u); /* bit 31 is derived, not writable */

    /* Master IRQ is derived from force-IRQ or enabled channel flags. */
    assert(psx_dma_write32(&dma, PSX_DMA_DICR_ADDR, 1u << 15u));
    assert(psx_dma_read32(&dma, PSX_DMA_DICR_ADDR, &value));
    assert((value & (1u << 31u)) != 0u);
    assert(psx_dma_write32(&dma, PSX_DMA_DICR_ADDR, 0u));
    assert(psx_dma_read32(&dma, PSX_DMA_DICR_ADDR, &value));
    assert((value & (1u << 31u)) == 0u);

    /* A channel flag only raises master IRQ when its channel and master
     * enables are both set. */
    dma.dicr = (1u << 23u) | (1u << 16u) | (1u << 24u);
    assert(psx_dma_read32(&dma, PSX_DMA_DICR_ADDR, &value));
    assert((value & (1u << 31u)) != 0u);
    dma.dicr = (1u << 23u) | (1u << 24u);
    assert(psx_dma_read32(&dma, PSX_DMA_DICR_ADDR, &value));
    assert((value & (1u << 31u)) == 0u);

    assert(!psx_dma_read32(&dma, PSX_DMA_BASE_ADDR + 0x0cu, &value));
    assert(!psx_dma_read32(&dma, PSX_DMA_BASE_ADDR + 0x70u, &value));
    assert(!psx_dma_read32(&dma, PSX_DMA_BASE_ADDR + 1u, &value));
    assert(!psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x0cu, 0u));

    psx_dma_reset(&dma);
    assert(psx_dma_read32(&dma, PSX_DMA_BASE_ADDR + 0x28u, &value));
    assert(value == 0u);
    assert(psx_dma_read32(&dma, PSX_DMA_DPCR_ADDR, &value));
    assert(value == 0x07654321u);

    puts("dma register tests passed");
    return 0;
}
