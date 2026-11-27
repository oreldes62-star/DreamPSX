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
    static uint16_t gpu_vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint32_t word = 0;
    uint16_t half = 0;

    psx_irq_reset(&irq);
    psx_timers_reset(&timers);
    assert(psx_gpu_init(&gpu, gpu_vram, PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    assert(psx_memory_init(&memory, ram, sizeof(ram), bios, sizeof(bios)));
    assert(psx_bus_init(&bus, &memory, &irq, &gpu, &timers));

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

    /* Unknown hardware registers and invalid alignments are rejected. */
    assert(!psx_bus_read32(&bus, 0x1f801080u, &word));
    assert(!psx_bus_read16(&bus, 0x00000101u, &half));
    assert(!psx_bus_write32(&bus, 0x1f801072u, 0u));
    assert(!psx_bus_read32(NULL, 0u, &word));

    puts("bus tests passed");
    return 0;
}
