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
    /* DPCR has one four-bit field per channel: priority[2:0], enable[3]. */
    assert(!psx_dma_channel_enabled(&dma, 2u));
    assert(psx_dma_channel_priority(&dma, 2u) == 3u);
    assert(!psx_dma_channel_enabled(&dma, PSX_DMA_CHANNEL_COUNT));
    assert(psx_dma_channel_priority(&dma, PSX_DMA_CHANNEL_COUNT) == 0u);
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x07654b21u));
    assert(psx_dma_channel_enabled(&dma, 2u));
    assert(psx_dma_channel_priority(&dma, 2u) == 3u);
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x07654321u));
    /* Among enabled pending channels, lower priority number wins. Ties
     * resolve deterministically to the lower channel index. */
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x0000aac0u));
    assert(psx_dma_select_channel(&dma, (1u << 1u) | (1u << 2u) |
                                  (1u << 3u)) == 2);
    assert(psx_dma_select_channel(&dma, 1u << 6u) == -1);
    assert(psx_dma_select_channel(&dma, 0u) == -1);
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x0000a2c0u));
    assert(psx_dma_select_channel(&dma, (1u << 1u) | (1u << 2u) |
                                  (1u << 3u)) == 3);
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x000092c0u));
    assert(psx_dma_select_channel(&dma, (1u << 1u) | (1u << 2u) |
                                  (1u << 3u)) == 3);
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x07654321u));

    /* CHCR requests persist in discovery while DPCR temporarily blocks
     * arbitration; enabling the channel makes it immediately selectable. */
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0u));
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x08u, 0x01000200u));
    assert(psx_dma_pending_mask(&dma) == 1u);
    assert(psx_dma_select_channel(&dma, psx_dma_pending_mask(&dma)) == -1);
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 8u));
    assert(psx_dma_select_channel(&dma, psx_dma_pending_mask(&dma)) == 0);
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x08u, 0u));

    /* The scheduler queue retains disabled requests and takes enabled
     * channels by priority, removing only the selected entry. */
    assert(psx_dma_enqueue_channel(&dma, 2u));
    assert(psx_dma_enqueue_channel(&dma, 1u));
    assert(psx_dma_enqueue_channel(&dma, 3u));
    assert(!psx_dma_enqueue_channel(&dma, PSX_DMA_CHANNEL_COUNT));
    assert(psx_dma_queued_mask(&dma) ==
           ((1u << 1u) | (1u << 2u) | (1u << 3u)));
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0u));
    assert(psx_dma_take_channel(&dma) == -1);
    assert(psx_dma_queued_mask(&dma) ==
           ((1u << 1u) | (1u << 2u) | (1u << 3u)));
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x0000aac0u));
    assert(psx_dma_take_channel(&dma) == 2);
    assert(psx_dma_queued_mask(&dma) == ((1u << 1u) | (1u << 3u)));
    assert(psx_dma_take_channel(&dma) == 1);
    assert(psx_dma_take_channel(&dma) == 3);
    assert(psx_dma_take_channel(&dma) == -1);
    assert(psx_dma_enqueue_channel(&dma, 4u));
    psx_dma_reset(&dma);
    assert(psx_dma_queued_mask(&dma) == 0u);

    /* Pending discovery honors CHCR start and sync mode; DPCR arbitration
     * is applied separately by the priority selector. */
    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x00008888u));
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x18u, 0x01000200u));
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x28u, 0x01000000u));
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x38u, 0x01000400u));
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x48u, 0x01000200u));
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x08u, 0x01000600u));
    assert(psx_dma_pending_mask(&dma) == ((1u << 1u) | (1u << 3u) | (1u << 4u)));
    assert(psx_dma_write32(&dma, PSX_DMA_BASE_ADDR + 0x28u, 0x11000000u));
    assert(psx_dma_pending_mask(&dma) ==
           ((1u << 1u) | (1u << 2u) | (1u << 3u) | (1u << 4u)));
    assert(psx_dma_select_channel(&dma, psx_dma_pending_mask(&dma)) == 1);
    assert(psx_dma_pending_mask(NULL) == 0u);

    /* Successful completion retires CHCR and latches the channel IRQ flag. */
    assert(psx_dma_write32(&dma, PSX_DMA_DICR_ADDR,
                           (1u << 23u) | (1u << 17u)));
    assert(psx_dma_complete_channel(&dma, 1u));
    assert(!psx_dma_complete_channel(&dma, PSX_DMA_CHANNEL_COUNT));
    assert(psx_dma_pending_mask(&dma) == ((1u << 2u) | (1u << 3u) | (1u << 4u)));
    assert(psx_dma_read32(&dma, PSX_DMA_BASE_ADDR + 0x18u, &value));
    assert((value & (1u << 24u)) == 0u);
    assert(psx_dma_read32(&dma, PSX_DMA_DICR_ADDR, &value));
    assert((value & (1u << 25u)) != 0u);
    assert((value & (1u << 31u)) != 0u);
    assert(psx_dma_write32(&dma, PSX_DMA_DICR_ADDR, 1u << 25u));
    assert(psx_dma_read32(&dma, PSX_DMA_DICR_ADDR, &value));
    assert((value & (1u << 31u)) == 0u);

    assert(psx_dma_write32(&dma, PSX_DMA_DPCR_ADDR, 0x07654321u));
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
    assert(value == 0u); /* DICR control bits are replaced by the write. */

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
    assert(psx_dma_read32(&dma, PSX_DMA_DPCR_ADDR, &value));
    assert(value == 0x0fffffffu);
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
