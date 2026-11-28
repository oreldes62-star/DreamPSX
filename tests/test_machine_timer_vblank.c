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
                                unsigned lines, bool pal, bool chunked)
{
    const uint16_t mode = (uint16_t)(
        PSX_TIMER_MODE_SYNC_ENABLE |
        (sync_mode << PSX_TIMER_MODE_SYNC_SHIFT) |
        (1u << PSX_TIMER_MODE_CLOCK_SHIFT));
    const uint32_t line_cycles = pal ? 2169u : 2153u;
    uint32_t remaining = line_cycles * lines;
    uint16_t counter = 0u;

    assert(psx_machine_set_video_clock_region(
        machine, pal ? PSX_VIDEO_CLOCK_PAL : PSX_VIDEO_CLOCK_NTSC));
    psx_machine_reset(machine);
    assert(psx_timers_write16(&machine->timers,
        PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u, mode));

    if (chunked) {
        const uint32_t chunks[] = {
            1u, line_cycles - 1u, 3u, 1024u, 4096u, 17u
        };
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

static uint16_t run_timer0_active_dot_ticks(PsxMachine *machine,
                                                  uint32_t horizontal_range,
                                                  bool pal, bool chunked)
{
    uint16_t counter = 0u;
    const uint16_t mode = (uint16_t)(
        PSX_TIMER_MODE_SYNC_ENABLE |
        (0u << PSX_TIMER_MODE_SYNC_SHIFT) |
        (1u << PSX_TIMER_MODE_CLOCK_SHIFT));

    assert(psx_machine_set_video_clock_region(
        machine, pal ? PSX_VIDEO_CLOCK_PAL : PSX_VIDEO_CLOCK_NTSC));
    psx_machine_reset(machine);
    assert(psx_gpu_gp1_write(&machine->gpu, 0x08000001u));
    if (horizontal_range != 0u)
        assert(psx_gpu_gp1_write(&machine->gpu, horizontal_range));
    assert(psx_timers_write16(&machine->timers,
        PSX_TIMER_BASE_ADDR + 4u, mode));

    if (chunked) {
        /* Split around the custom [1024,2048) GP1(06h) edges:
         * NTSC: 646/1292 CPU cycles; PAL: 652/1304 CPU cycles.
         */
        const uint32_t ntsc_default[] = {322u, 1u, 1614u, 1u, 215u};
        const uint32_t pal_default[] = {325u, 1u, 1629u, 1u, 213u};
        const uint32_t ntsc_custom[] = {645u, 1u, 645u, 1u, 861u};
        const uint32_t pal_custom[] = {651u, 1u, 651u, 1u, 865u};
        const uint32_t *chunks;
        if (horizontal_range == 0u)
            chunks = pal ? pal_default : ntsc_default;
        else
            chunks = pal ? pal_custom : ntsc_custom;
        for (size_t i = 0u; i < 5u; ++i)
            assert(psx_machine_advance_system_cycles(machine, chunks[i]));
    } else {
        assert(psx_machine_advance_system_cycles(machine, pal ? 2169u : 2153u));
    }

    assert(psx_timers_read16(&machine->timers,
        PSX_TIMER_BASE_ADDR, &counter));
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
    assert(run_timer1_mode(&machine, 0u, 16u, false, false) == 1u);
    assert(run_timer1_mode(&machine, 0u, 17u, false, false) == 2u);

    /* Mode 1 resets at the VBlank rising edge but continues to count. */
    assert(run_timer1_mode(&machine, 1u, 16u, false, false) == 16u);
    assert(run_timer1_mode(&machine, 1u, 17u, false, false) == 17u);

    /* Mode 2 resets at the VBlank rising edge and counts only while
     * VBlank is active: lines 1 through 15 in this startup window.
     */
    assert(run_timer1_mode(&machine, 2u, 16u, false, false) == 15u);

    /* Mode 3 waits for the first VBlank edge, then free-runs. */
    assert(run_timer1_mode(&machine, 3u, 16u, false, false) == 16u);
    assert(run_timer1_mode(&machine, 3u, 17u, false, false) == 17u);

    /* Timer 1's VBlank synchronization and HBlank clock pulse cadence
     * must produce the same line counts on the PAL physical oscillator.
     */
    for (unsigned sync_mode = 0u; sync_mode < 4u; ++sync_mode) {
        const uint16_t ntsc = run_timer1_mode(
            &machine, sync_mode, 17u, false, false);
        const uint16_t pal = run_timer1_mode(
            &machine, sync_mode, 17u, true, false);
        const uint16_t pal_chunked = run_timer1_mode(
            &machine, sync_mode, 17u, true, true);
        assert(pal == ntsc);
        assert(pal_chunked == pal);
    }

    /* GP1(08h) resets the GPU output timing state. Timer 1 must see
     * the corresponding VBlank level change immediately on the next
     * machine-clock advance, rather than remaining gated by the old field.
     */
    psx_machine_reset(&machine);
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u,
        PSX_TIMER_MODE_SYNC_ENABLE));
    assert(psx_timers_set_sync_signal(&machine.timers, 1u, true));
    assert(machine.timers.timer[1].sync_signal_active);
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000001u));
    assert(psx_machine_advance_system_cycles(&machine, 1u));
    assert(!machine.timers.timer[1].sync_signal_active);
    assert(machine.timers.timer[1].counter == 1u);

    /* Timer 0 counts GPU dots only while HBlank is inactive in sync
     * mode 0. GP1(06h)'s default [512,3072) interval is 2560 video
     * clocks, or exactly 320 dots in 320-pixel mode (divider 8).
     */
    assert(run_timer0_active_dot_ticks(&machine, 0u, false, false) == 320u);
    assert(run_timer0_active_dot_ticks(&machine, 0u, false, true) == 320u);
    /* The default interval must produce the same number of GPU dots on
     * the PAL oscillator, even when execution is split at both HBlank edges.
     */
    assert(run_timer0_active_dot_ticks(&machine, 0u, true, false) == 320u);
    assert(run_timer0_active_dot_ticks(&machine, 0u, true, true) == 320u);

    /* A custom [1024,2048) display interval is 1024 video clocks,
     * therefore 128 Timer 0 dots in 320-pixel mode.
     */
    assert(run_timer0_active_dot_ticks(&machine, 0x06800400u,
                                        false, false) == 128u);
    assert(run_timer0_active_dot_ticks(&machine, 0x06800400u,
                                        false, true) == 128u);
    /* Repeat the custom range with the PAL physical oscillator. The
     * CPU-cycle boundaries differ, but the GPU-dot count must not.
     */
    assert(run_timer0_active_dot_ticks(&machine, 0x06800400u,
                                        true, false) == 128u);
    assert(run_timer0_active_dot_ticks(&machine, 0x06800400u,
                                        true, true) == 128u);

    /* Every tested mode must be invariant to how master-clock cycles are
     * batched around the raster's scanline boundaries. */
    for (unsigned sync_mode = 0u; sync_mode < 4u; ++sync_mode) {
        const uint16_t bulk = run_timer1_mode(&machine, sync_mode, 17u, false, false);
        const uint16_t chunked = run_timer1_mode(&machine, sync_mode, 17u, false, true);
        assert(chunked == bulk);
    }

    free(vram);
    free(bios);
    free(ram);
    return 0;
}
