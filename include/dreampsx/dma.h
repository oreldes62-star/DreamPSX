#ifndef DREAMPSX_DMA_H
#define DREAMPSX_DMA_H

#include <stdbool.h>
#include <stdint.h>

#define PSX_DMA_BASE_ADDR 0x1f801080u
#define PSX_DMA_DPCR_ADDR 0x1f8010f0u
#define PSX_DMA_DICR_ADDR 0x1f8010f4u
#define PSX_DMA_CHANNEL_COUNT 7u

typedef struct {
    uint32_t madr;
    uint32_t bcr;
    uint32_t chcr;
} PsxDmaChannel;

typedef struct {
    PsxDmaChannel channel[PSX_DMA_CHANNEL_COUNT];
    uint32_t dpcr;
    uint32_t dicr;
    bool initialized;
} PsxDmaController;

void psx_dma_reset(PsxDmaController *dma);
bool psx_dma_init(PsxDmaController *dma);
bool psx_dma_read32(const PsxDmaController *dma, uint32_t address,
                    uint32_t *value);
bool psx_dma_write32(PsxDmaController *dma, uint32_t address,
                     uint32_t value);

#endif
