#include "dreampsx/machine.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    PsxMachine machine;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    uint16_t *vram = (uint16_t *)calloc(
        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT, sizeof(uint16_t));
    uint16_t half = 0u;
    uint32_t word = 0u;
    assert(ram && bios && vram);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios,
                            PSX_BIOS_SIZE, vram,
                            PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* Root-counter registers are reachable through physical and KSEG1 bus. */
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 3u));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u,
                           PSX_TIMER_MODE_RESET_AT_TARGET));
    assert(psx_timers_advance(&machine.timers, 0u, 3u));
    assert(psx_bus_read16(&machine.bus, 0xbf801100u, &half));
    assert(half == 0u);
    assert(psx_bus_read32(&machine.bus, PSX_TIMER_BASE_ADDR + 4u, &word));
    assert((word & PSX_TIMER_MODE_REACHED_TARGET) != 0u);

    /* Machine reset clears all timer registers. */
    psx_machine_reset(&machine);
    assert(psx_bus_read16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, &half));
    assert(half == 0u);
    assert(psx_bus_read16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u, &half));
    assert(half == 0u);

    /* Unmapped and misaligned timer register accesses remain bus faults. */
    assert(!psx_bus_read16(&machine.bus, PSX_TIMER_BASE_ADDR + 2u, &half));
    assert(!psx_bus_write32(&machine.bus, PSX_TIMER_BASE_ADDR + 12u, 0u));

    free(vram);
    free(bios);
    free(ram);
    puts("DreamPSX timer bus tests: OK");
    return 0;
}
