#ifndef DREAMPSX_DMA_H
#define DREAMPSX_DMA_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/memory.h"
#include "dreampsx/gpu.h"

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
/* Execute a triggered channel-2 manual RAM-to-GP0 transfer. */
bool psx_dma_execute_gpu_manual(PsxDmaController *dma,
                                 PsxMemory *memory, PsxGpu *gpu);
/* Execute channel-2 RAM-to-GP0 linked-list mode (CHCR sync=2). */
bool psx_dma_execute_gpu_linked_list(PsxDmaController *dma,
                                     PsxMemory *memory, PsxGpu *gpu);
/* Execute channel-2 request/block mode (CHCR sync=1), in either direction. */
bool psx_dma_execute_gpu_request(PsxDmaController *dma,
                                  PsxMemory *memory, PsxGpu *gpu);

#endif
