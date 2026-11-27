#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dreampsx/bus.h"

int main(void)
{
    uint8_t ram[PSX_RAM_SIZE] = {0};
    uint8_t bios[PSX_BIOS_SIZE] = {0};
    PsxMemory memory = {0};
    PsxIrqController irq;
    PsxBus bus = {0};
    PsxGpu gpu;
    PsxTimers timers;
    PsxSpu spu;
    static uint16_t gpu_vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint32_t word = 0;
    uint16_t half = 0;

    psx_irq_reset(&irq);
    psx_timers_reset(&timers);
    assert(psx_gpu_init(&gpu, gpu_vram, PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    assert(psx_memory_init(&memory, ram, sizeof(ram), bios, sizeof(bios)));
    assert(psx_bus_init(&bus, &memory, &irq, &gpu, &timers));
    assert(psx_spu_init(&spu));
    bus.spu = &spu;

    /* RAM is visible through physical, cached and uncached segments. */
    assert(psx_bus_write32(&bus, 0x00000100u, 0x12345678u));
    assert(psx_bus_read32(&bus, 0x80000100u, &word));
    assert(word == 0x12345678u);
    assert(psx_bus_write16(&bus, 0xa0000102u, 0xabcd));
    assert(psx_bus_read32(&bus, 0x00000100u, &word));
    assert(word == 0xabcd5678u);

    /* IRQ registers are routed through the same bus, including KSEG1. */
    assert(psx_bus_write16(&bus, 0xbf801074u, 0x0020u));
    psx_irq_set_line(&irq, 5u, true);
    assert(psx_bus_read16(&bus, 0x1f801070u, &half));
    assert((half & 0x0020u) != 0u);
    assert(psx_bus_read32(&bus, 0xbf801070u, &word));
    assert((word & 0xffffu) == half);

    /* I_STAT is write-zero-to-clear: writing zero clears the pending bit. */
    assert(psx_bus_write16(&bus, 0xbf801070u, (uint16_t)~0x0020u));
    assert(psx_bus_read16(&bus, 0x1f801070u, &half));
    assert((half & 0x0020u) == 0u);

    /* GPU GP0/GP1 ports are reachable through the shared CPU bus. */
    assert(psx_bus_write32(&bus, 0x1f801810u, 0x020000ffu));
    assert(psx_bus_write32(&bus, 0x1f801810u, 0u));
    assert(psx_bus_write32(&bus, 0x1f801810u, 1u | (1u << 16u)));
    assert(psx_gpu_vram_read(&gpu, 0u, 0u, &half));
    assert(half == 0x001fu);
    assert(psx_bus_read32(&bus, 0x1f801814u, &word));
    assert(word == 0x14802000u);

    /* GP0(1Fh) asserts GPU IRQ1 through the bus; GP1(02h) drops
     * the source line, while I_STAT remains latched until acknowledged. */
    assert(psx_bus_write16(&bus, 0x1f801074u, (uint16_t)(1u << 1u)));
    assert(psx_bus_write32(&bus, 0x1f801810u, 0x1f000000u));
    assert(psx_gpu_irq_output(&gpu));
    assert((irq.source_level & (1u << 1u)) != 0u);
    assert((psx_irq_read_stat(&irq) & (1u << 1u)) != 0u);
    assert(psx_bus_read32(&bus, 0x1f801814u, &word));
    assert((word & (1u << 24u)) != 0u);
    assert(psx_bus_write32(&bus, 0x1f801814u, 0x02000000u));
    assert(!psx_gpu_irq_output(&gpu));
    assert((irq.source_level & (1u << 1u)) == 0u);
    assert((psx_irq_read_stat(&irq) & (1u << 1u)) != 0u);
    assert(psx_bus_write16(&bus, 0x1f801070u,
                           (uint16_t)~(1u << 1u)));
    assert((psx_irq_read_stat(&irq) & (1u << 1u)) == 0u);

    /* GP0 A0 upload and C0 readback are exposed through GP0/GPUREAD MMIO. */
    assert(psx_bus_write32(&bus, 0xbf801810u, 0xa0000000u));
    assert(psx_bus_write32(&bus, 0xbf801810u, 8u | (9u << 16u)));
    assert(psx_bus_write32(&bus, 0xbf801810u, 2u | (1u << 16u)));
    assert(psx_bus_write32(&bus, 0xbf801810u, 0x56781234u));
    assert(psx_bus_write32(&bus, 0x1f801810u, 0xc0000000u));
    assert(psx_bus_write32(&bus, 0x1f801810u, 8u | (9u << 16u)));
    assert(psx_bus_write32(&bus, 0x1f801810u, 2u | (1u << 16u)));
    assert(psx_bus_read32(&bus, 0xbf801810u, &word));
    assert(word == 0x56781234u);

    /* SPU registers and transfer RAM share the CPU bus mapping. */
    assert(psx_bus_write16(&bus, PSX_SPU_BASE_ADDR + 0x0e0u, 0x4567u));
    assert(psx_bus_read16(&bus, PSX_SPU_BASE_ADDR + 0x0e0u, &half));
    assert(half == 0x4567u);
    assert(psx_bus_write16(&bus, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0020u));
    assert(psx_bus_write32(&bus, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0x89abcdefu));
    assert(psx_bus_write16(&bus, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0020u));
    assert(psx_bus_read32(&bus, PSX_SPU_BASE_ADDR +
                          PSX_SPU_TRANSFER_DATA_OFFSET, &word));
    assert(word == 0x89abcdefu);
    assert(spu.transfer_address == 0x104u);
    assert(psx_bus_write16(&bus, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_ON_LOW_OFFSET, 0x0005u));
    assert(psx_spu_active_voice_mask(&spu) == 0x00000005u);
    assert(psx_bus_write16(&bus, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0001u));
    assert(psx_spu_active_voice_mask(&spu) == 0x00000004u);

    /* Unknown hardware registers and invalid alignments are rejected. */
    assert(!psx_bus_read32(&bus, 0x1f801080u, &word));
    assert(!psx_bus_read16(&bus, 0x00000101u, &half));
    assert(!psx_bus_write32(&bus, 0x1f801072u, 0u));
    assert(!psx_bus_read32(NULL, 0u, &word));

    puts("bus tests passed");
    return 0;
}
