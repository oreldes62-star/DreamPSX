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

/* Channel 2 manual mode: RAM-to-GP0 and GPUREAD-to-RAM. This is
 * intentionally synchronous; arbitration and cycle timing come later. */
bool psx_dma_execute_gpu_manual(PsxDmaController *dma,
                                 PsxMemory *memory, PsxGpu *gpu)
{
    PsxDmaChannel *channel;
    uint32_t control;
    uint32_t words;
    uint32_t address;
    int32_t step;
    bool ram_to_gpu;

    if (!dma || !dma->initialized || !memory || !memory->initialized ||
        !gpu || !gpu->initialized)
        return false;

    channel = &dma->channel[2];
    control = channel->chcr;
    ram_to_gpu = (control & 1u) != 0u;

    /* Manual sync mode requires start, trigger and channel-2 DPCR enable. */
    if ((control & (1u << 24u)) == 0u ||
        (control & (1u << 28u)) == 0u ||
        ((control >> 9u) & 3u) != 0u ||
        (dma->dpcr & (1u << 11u)) == 0u)
        return true;

    if (ram_to_gpu) {
        if (gpu->dma_direction != 2u) return true;
    } else {
        if (gpu->dma_direction != 3u ||
            gpu->transfer_mode != PSX_GPU_TRANSFER_VRAM_TO_CPU)
            return false;
    }

    words = channel->bcr & 0xffffu;
    if (words == 0u) words = 0x10000u;
    address = channel->madr;
    step = (control & 2u) != 0u ? -4 : 4;

    for (uint32_t i = 0u; i < words; ++i) {
        uint32_t data;
        if (ram_to_gpu) {
            if (!psx_memory_read32(memory, address, &data))
                return false;
            if (!psx_gpu_gp0_write(gpu, data))
                return false;
        } else {
            /* GPUREAD returns zero when idle, so stop rather than fabricating
             * words after the image-transfer payload has been exhausted. */
            if (gpu->transfer_mode != PSX_GPU_TRANSFER_VRAM_TO_CPU)
                return false;
            if (!psx_gpu_read_data(gpu, &data) ||
                !psx_memory_write32(memory, address, data))
                return false;
        }
        address = (uint32_t)(address + step) & 0x00fffffcu;
    }

    channel->madr = address;
    channel->bcr &= 0xffff0000u;
    channel->chcr &= ~((1u << 24u) | (1u << 28u));
    dma->dicr |= 1u << 26u; /* Channel 2 completion flag. */
    return true;
}


/* Channel 2 GPU linked-list mode. Packet headers are never sent to GP0:
 * bits 31..24 contain the payload word count and bits 23..0 the next
 * header address. The all-ones 24-bit pointer terminates the chain. */
bool psx_dma_execute_gpu_linked_list(PsxDmaController *dma,
                                     PsxMemory *memory, PsxGpu *gpu)
{
    PsxDmaChannel *channel;
    uint32_t control;
    uint32_t address;
    uint32_t packet_count = 0u;
    uint32_t total_words = 0u;

    if (!dma || !dma->initialized || !memory || !memory->initialized ||
        !gpu || !gpu->initialized)
        return false;

    channel = &dma->channel[2];
    control = channel->chcr;

    /* Linked-list mode is device-directed RAM-to-GP0 only. CHCR bit 28
     * is a manual-mode trigger and is deliberately not required here. */
    if ((control & (1u << 24u)) == 0u ||
        (control & 1u) == 0u ||
        ((control >> 9u) & 3u) != 2u ||
        (dma->dpcr & (1u << 11u)) == 0u)
        return true;
    if (gpu->dma_direction != 2u) return false;

    address = channel->madr & 0x00fffffcu;
    for (;;) {
        uint32_t header;
        uint32_t count;
        uint32_t next;
        uint32_t payload_address;

        if (++packet_count > 65536u) return false;
        channel->madr = address;
        if (!psx_memory_read32(memory, address, &header)) return false;

        count = header >> 24u;
        next = header & 0x00ffffffu;
        payload_address = (address + 4u) & 0x00fffffcu;

        if (total_words > 1048576u - count) return false;
        for (uint32_t i = 0u; i < count; ++i) {
            uint32_t data;
            if (!psx_memory_read32(memory, payload_address, &data) ||
                !psx_gpu_gp0_write(gpu, data))
                return false;
            payload_address = (payload_address + 4u) & 0x00fffffcu;
            ++total_words;
        }

        if (next == 0x00ffffffu) {
            channel->madr = 0x00ffffffu;
            channel->chcr &= ~(1u << 24u);
            dma->dicr |= 1u << 26u;
            return true;
        }
        if ((next & 3u) != 0u) return false;
        address = next;
    }
}


/* Channel 2 request mode transfers block_size * block_count words.
 * This first implementation is synchronous and models the programmed
 * block geometry; FIFO arbitration and per-block GPU request stalls are
 * intentionally deferred. */
bool psx_dma_execute_gpu_request(PsxDmaController *dma,
                                  PsxMemory *memory, PsxGpu *gpu)
{
    PsxDmaChannel *channel;
    uint32_t control;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t words;
    uint32_t address;
    int32_t step;
    bool ram_to_gpu;

    if (!dma || !dma->initialized || !memory || !memory->initialized ||
        !gpu || !gpu->initialized)
        return false;

    channel = &dma->channel[2];
    control = channel->chcr;
    if ((control & (1u << 24u)) == 0u ||
        ((control >> 9u) & 3u) != 1u ||
        (dma->dpcr & (1u << 11u)) == 0u)
        return true;

    ram_to_gpu = (control & 1u) != 0u;
    if (ram_to_gpu) {
        if (gpu->dma_direction != 2u) return false;
    } else if (gpu->dma_direction != 3u ||
               gpu->transfer_mode != PSX_GPU_TRANSFER_VRAM_TO_CPU) {
        return false;
    }

    block_size = channel->bcr & 0xffffu;
    block_count = channel->bcr >> 16u;
    if (block_size == 0u) block_size = 0x10000u;
    if (block_count == 0u) block_count = 0x10000u;
    if (block_size > 1048576u / block_count) return false;
    words = block_size * block_count;
    if (words > 1048576u) return false;

    address = channel->madr;
    step = (control & 2u) != 0u ? -4 : 4;
    for (uint32_t i = 0u; i < words; ++i) {
        uint32_t data;
        if (ram_to_gpu) {
            if (!psx_memory_read32(memory, address, &data) ||
                !psx_gpu_gp0_write(gpu, data))
                return false;
        } else {
            if (gpu->transfer_mode != PSX_GPU_TRANSFER_VRAM_TO_CPU ||
                !psx_gpu_read_data(gpu, &data) ||
                !psx_memory_write32(memory, address, data))
                return false;
        }
        address = (uint32_t)(address + step) & 0x00fffffcu;
    }

    channel->madr = address;
    channel->bcr = 0u;
    channel->chcr &= ~(1u << 24u);
    dma->dicr |= 1u << 26u;
    return true;
}
