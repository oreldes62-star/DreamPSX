#include "dreampsx/machine.h"

#include <assert.h>
#include <stdlib.h>

static void init_machine(PsxMachine *machine, uint8_t **ram_out,
                         uint8_t **bios_out, uint16_t **vram_out)
{
    *ram_out = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    *bios_out = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    *vram_out = (uint16_t *)calloc(
        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT, sizeof(uint16_t));
    assert(*ram_out && *bios_out && *vram_out);
    assert(psx_machine_init(machine, *ram_out, PSX_RAM_SIZE,
                            *bios_out, PSX_BIOS_SIZE, *vram_out,
                            PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
}

static uint16_t run_timer1_mode(PsxMachine *machine, unsigned sync_mode,
                                unsigned lines, bool chunked)
{
    const uint16_t mode = (uint16_t)(
        PSX_TIMER_MODE_SYNC_ENABLE |
        (sync_mode << PSX_TIMER_MODE_SYNC_SHIFT) |
        (1u << PSX_TIMER_MODE_CLOCK_SHIFT));
    const uint32_t line_cycles = 2148u;
    uint32_t remaining = line_cycles * lines;
    uint16_t counter = 0u;

    psx_machine_reset(machine);
    assert(psx_timers_write16(&machine->timers,
        PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u, mode));

    if (chunked) {
        const uint32_t chunks[] = {1u, 2147u, 3u, 1024u, 4096u, 17u};
        for (size_t i = 0u; i < sizeof(chunks) / sizeof(chunks[0]); ++i) {
            const uint32_t step = chunks[i] < remaining ?
                                  chunks[i] : remaining;
            if (step != 0u) {
                assert(psx_machine_advance_system_cycles(machine, step));
                remaining -= step;
            }
        }
    }
    if (remaining != 0u)
        assert(psx_machine_advance_system_cycles(machine, remaining));

    assert(psx_timers_read16(&machine->timers,
        PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE, &counter));
    return counter;
}

int main(void)
{
    PsxMachine machine;
    uint8_t *ram;
    uint8_t *bios;
    uint16_t *vram;
    init_machine(&machine, &ram, &bios, &vram);

    /* Timer 1 synchronization is driven by VBlank, while its selected
     * external clock is HBlank (one tick per scanline). */
    assert(run_timer1_mode(&machine, 0u, 16u, false) == 1u);
    assert(run_timer1_mode(&machine, 0u, 17u, false) == 2u);

    /* Mode 1 resets at the VBlank rising edge but continues to count. */
    assert(run_timer1_mode(&machine, 1u, 16u, false) == 16u);
    assert(run_timer1_mode(&machine, 1u, 17u, false) == 17u);

    /* Mode 3 waits for the first VBlank edge, then free-runs. */
    assert(run_timer1_mode(&machine, 3u, 16u, false) == 16u);
    assert(run_timer1_mode(&machine, 3u, 17u, false) == 17u);

    /* Every tested mode must be invariant to how master-clock cycles are
     * batched around the raster's scanline boundaries. */
    for (unsigned sync_mode = 0u; sync_mode < 4u; ++sync_mode) {
        const uint16_t bulk = run_timer1_mode(&machine, sync_mode, 17u, false);
        const uint16_t chunked = run_timer1_mode(&machine, sync_mode, 17u, true);
        assert(chunked == bulk);
    }

    free(vram);
    free(bios);
    free(ram);
    return 0;
}
