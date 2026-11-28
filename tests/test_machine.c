#include "dreampsx/machine.h"
#include "dreampsx/cop0_irq.h"
#include "dreampsx/dreamcast_audio_stream.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

int main(void)
{
    PsxMachine machine;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    uint16_t *gpu_vram = (uint16_t *)calloc(PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT, sizeof(uint16_t));
    assert(ram && bios && gpu_vram);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE,
                            gpu_vram, PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    /* Timer 0 receives the GPU dot clock with a fractional master-clock
     * phase that is stable across batching and resets when GP1 mode changes.
     */
    {
        const uint32_t chunks[] = {3u, 7u, 1u, 14u};
        uint16_t counter = 0u;

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u,
            (uint16_t)(1u << PSX_TIMER_MODE_CLOCK_SHIFT)));
        assert(psx_machine_advance_system_cycles(&machine, 25u));
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR, &counter));
        assert(counter == 2u);
        assert(machine.timer0_dot_phase == 5u);

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u,
            (uint16_t)(1u << PSX_TIMER_MODE_CLOCK_SHIFT)));
        for (size_t i = 0u; i < sizeof(chunks) / sizeof(chunks[0]); ++i)
            assert(psx_machine_advance_system_cycles(&machine, chunks[i]));
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR, &counter));
        assert(counter == 2u);
        assert(machine.timer0_dot_phase == 5u);

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u,
            (uint16_t)(1u << PSX_TIMER_MODE_CLOCK_SHIFT)));
        assert(psx_machine_advance_system_cycles(&machine, 9u));
        assert(machine.timer0_dot_phase == 9u);
        assert(psx_gpu_gp1_write(&machine.gpu, 0x08000001u)); /* 320 / 8 */
        assert(psx_machine_advance_system_cycles(&machine, 8u));
        assert(machine.timer0_dot_phase == 0u);
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR, &counter));
        assert(counter == 1u);

        /* All five supported horizontal modes feed their own dot divisor. */
        {
            static const unsigned modes[] = {0u, 1u, 2u, 3u, 0x40u};
            static const unsigned divisors[] = {10u, 8u, 5u, 4u, 7u};
            for (size_t i = 0u; i < sizeof(modes) / sizeof(modes[0]); ++i) {
                psx_machine_reset(&machine);
                assert(psx_gpu_gp1_write(&machine.gpu,
                    0x08000000u | modes[i]));
                assert(psx_timers_write16(&machine.timers,
                    PSX_TIMER_BASE_ADDR + 4u,
                    (uint16_t)(1u << PSX_TIMER_MODE_CLOCK_SHIFT)));
                assert(psx_machine_advance_system_cycles(
                    &machine, divisors[i] * 4u - 1u));
                assert(psx_timers_read16(&machine.timers,
                    PSX_TIMER_BASE_ADDR, &counter));
                assert(counter == 3u);
                assert(machine.timer0_dot_phase == divisors[i] - 1u);
            }
        }
    }

    /* Machine-level GPU raster timing must be invariant to cycle batching. */
    {
        const uint32_t total_cycles = 2148u * 3u + 517u;
        const uint32_t chunks[] = {1u, 37u, 211u, 1024u, 3000u, 669u};
        unsigned remaining;
        uint32_t scanline;
        uint32_t phase;
        uint64_t frames;
        bool vblank;

        assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
        assert(psx_machine_advance_system_cycles(&machine, total_cycles));
        scanline = machine.gpu.display.video_scanline;
        phase = machine.gpu_scanline_cycle_phase;
        frames = machine.gpu.display.video_frame_counter;
        vblank = machine.gpu.display.video_vblank;

        psx_machine_reset(&machine);
        assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
        remaining = total_cycles;
        for (unsigned i = 0u; i < sizeof(chunks) / sizeof(chunks[0]); ++i) {
            const uint32_t step = chunks[i] < remaining ?
                                  chunks[i] : remaining;
            if (step != 0u) {
                assert(psx_machine_advance_system_cycles(&machine, step));
                remaining -= step;
            }
        }
        if (remaining != 0u)
            assert(psx_machine_advance_system_cycles(&machine, remaining));
        assert(machine.gpu.display.video_scanline == scanline);
        assert(machine.gpu_scanline_cycle_phase == phase);
        assert(machine.gpu.display.video_frame_counter == frames);
        assert(machine.gpu.display.video_vblank == vblank);
    }
    /* Timer 1 receives exactly one HBlank clock per completed scanline,
     * independent of how the master-clock interval is batched. */
    {
        const uint32_t total_cycles = 2148u * 5u + 123u;
        const uint32_t chunks[] = {17u, 1000u, 3001u, 2048u, 3000u, 2100u};
        uint32_t expected_counter;
        uint16_t expected_irq_status;
        uint32_t remaining = total_cycles;
        uint16_t counter = 0u;
        const uint16_t timer1_mode =
            (uint16_t)((1u << PSX_TIMER_MODE_CLOCK_SHIFT) |
                       PSX_TIMER_MODE_IRQ_TARGET);

        psx_machine_reset(&machine);
        assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 8u, 3u));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u, timer1_mode));
        assert(psx_machine_advance_system_cycles(&machine, total_cycles));
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE, &counter));
        expected_counter = counter;
        expected_irq_status = psx_irq_read_stat(&machine.irq);
        assert(expected_counter == 5u);
        assert((expected_irq_status & (1u << 5u)) != 0u);

        psx_machine_reset(&machine);
        assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 8u, 3u));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u, timer1_mode));
        for (unsigned i = 0u; i < sizeof(chunks) / sizeof(chunks[0]); ++i) {
            const uint32_t step = chunks[i] < remaining ? chunks[i] : remaining;
            if (step != 0u) {
                assert(psx_machine_advance_system_cycles(&machine, step));
                remaining -= step;
            }
        }
        if (remaining != 0u)
            assert(psx_machine_advance_system_cycles(&machine, remaining));
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE, &counter));
        assert(counter == expected_counter);
        assert(psx_irq_read_stat(&machine.irq) == expected_irq_status);
        assert((expected_irq_status & (1u << 5u)) != 0u);

        /* Reset-at-target must restart the counter on HBlank target hits. */
        psx_machine_reset(&machine);
        assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 8u, 3u));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u,
            (uint16_t)(timer1_mode | PSX_TIMER_MODE_RESET_AT_TARGET)));
        assert(psx_machine_advance_system_cycles(&machine, total_cycles));
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE, &counter));
        assert(counter == 2u);
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u, &expected_irq_status));
        assert((expected_irq_status & PSX_TIMER_MODE_REACHED_TARGET) != 0u);
        assert((psx_irq_read_stat(&machine.irq) & (1u << 5u)) != 0u);

        /* The reset-at-target path must also be invariant to cycle batching. */
        {
            const uint32_t reset_chunks[] = {1u, 2147u, 1u, 17u, 2000u,
                                             2148u, 19u};
            uint16_t bulk_reset_counter = 0u;
            uint16_t bulk_reset_mode = 0u;
            uint16_t bulk_reset_irq = 0u;
            uint16_t chunk_reset_counter = 0u;
            uint32_t reset_remaining = 2148u * 2u + 19u;

            psx_machine_reset(&machine);
            assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
            assert(psx_timers_write16(&machine.timers,
                PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 8u, 3u));
            assert(psx_timers_write16(&machine.timers,
                PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u,
                (uint16_t)(timer1_mode | PSX_TIMER_MODE_RESET_AT_TARGET)));
            assert(psx_machine_advance_system_cycles(&machine,
                                                      reset_remaining));
            assert(psx_timers_read16(&machine.timers,
                PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE,
                &bulk_reset_counter));
            bulk_reset_mode = machine.timers.timer[1].mode;
            bulk_reset_irq = psx_irq_read_stat(&machine.irq);

            psx_machine_reset(&machine);
            assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
            assert(psx_timers_write16(&machine.timers,
                PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 8u, 3u));
            assert(psx_timers_write16(&machine.timers,
                PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u,
                (uint16_t)(timer1_mode | PSX_TIMER_MODE_RESET_AT_TARGET)));
            for (size_t i = 0u;
                 i < sizeof(reset_chunks) / sizeof(reset_chunks[0]); ++i) {
                const uint32_t step = reset_chunks[i] < reset_remaining ?
                                      reset_chunks[i] : reset_remaining;
                if (step != 0u) {
                    assert(psx_machine_advance_system_cycles(&machine, step));
                    reset_remaining -= step;
                }
            }
            if (reset_remaining != 0u)
                assert(psx_machine_advance_system_cycles(&machine,
                                                          reset_remaining));
            assert(psx_timers_read16(&machine.timers,
                PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE,
                &chunk_reset_counter));
            assert(chunk_reset_counter == bulk_reset_counter);
            assert(machine.timers.timer[1].mode == bulk_reset_mode);
            assert(psx_irq_read_stat(&machine.irq) == bulk_reset_irq);
        }
    }
    /* Timer 1 VBlank synchronization is wired to the GPU raster.
     * With sync mode 2 it counts HBlank ticks during VBlank only. The
     * default visible range starts at scanline 16: lines 1..15 are blank,
     * then line 16 leaves VBlank and must stop the counter.
     */
    {
        uint16_t counter = 0u;
        const uint16_t mode = (uint16_t)(
            PSX_TIMER_MODE_SYNC_ENABLE |
            (2u << PSX_TIMER_MODE_SYNC_SHIFT) |
            (1u << PSX_TIMER_MODE_CLOCK_SHIFT));

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u, mode));
        assert(psx_machine_advance_system_cycles(&machine, 2148u * 16u));
        assert(machine.gpu.display.video_scanline == 16u);
        assert(!machine.gpu.display.video_vblank);
        assert(!machine.timers.timer[1].sync_signal_active);
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE, &counter));
        assert(counter == 15u);
    }

    /* Timer 0 mode 1 resets on the end-of-display HBlank edge, then
     * continues counting through blanking and the next visible interval.
     */
    {
        uint16_t counter = 0u;
        const uint16_t mode = (uint16_t)(
            PSX_TIMER_MODE_SYNC_ENABLE |
            (1u << PSX_TIMER_MODE_SYNC_SHIFT));

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u, mode));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR, 77u));
        assert(psx_machine_advance_system_cycles(&machine, 2148u * 2u + 10u));
        assert(machine.timers.timer[0].sync_started);
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR, &counter));
        assert(counter == 225u);
    }

    /* Timer 0 modes 0 and 2 must gate system-clock ticks over the
     * horizontal blanking interval, independent of CPU batch boundaries.
     * With the default GP1(06h) range, the modeled visible interval is
     * 1611 cycles and the two blanking portions total 537 cycles per line.
     */
    {
        const uint32_t chunks[] = {
            322u, 1612u, 214u, 322u, 1612u, 224u
        };
        const uint16_t modes[] = {
            PSX_TIMER_MODE_SYNC_ENABLE,
            (uint16_t)(PSX_TIMER_MODE_SYNC_ENABLE |
                       (2u << PSX_TIMER_MODE_SYNC_SHIFT))
        };
        const uint16_t expected_mode0 = 3222u;

        for (size_t mode_index = 0u; mode_index < 2u; ++mode_index) {
            uint16_t bulk_counter = 0u;
            uint16_t chunk_counter = 0u;
            psx_machine_reset(&machine);
            assert(psx_timers_write16(&machine.timers,
                PSX_TIMER_BASE_ADDR + 4u, modes[mode_index]));
            assert(psx_machine_advance_system_cycles(
                &machine, 2148u * 2u + 10u));
            assert(psx_timers_read16(&machine.timers,
                PSX_TIMER_BASE_ADDR, &bulk_counter));
            if (mode_index == 0u)
                assert(bulk_counter == expected_mode0);
            else
                /* Mode 2 resets at each HBlank rising edge, so the final
                 * partial blanking interval determines the residual count. */
                assert(bulk_counter > 0u && bulk_counter < 4306u);

            psx_machine_reset(&machine);
            assert(psx_timers_write16(&machine.timers,
                PSX_TIMER_BASE_ADDR + 4u, modes[mode_index]));
            for (size_t i = 0u; i < sizeof(chunks) / sizeof(chunks[0]); ++i)
                assert(psx_machine_advance_system_cycles(&machine, chunks[i]));
            assert(psx_timers_read16(&machine.timers,
                PSX_TIMER_BASE_ADDR, &chunk_counter));
            assert(chunk_counter == bulk_counter);
            assert(machine.gpu_scanline_cycle_phase == 10u);
        }
    }

    /* HBlank synchronization must remain batch-invariant in every
     * supported GPU horizontal mode, including changes to the dot divisor.
     * The modeled GP1(06h) interval is shared, but dot-clock phase must not
     * leak into the system-clock Timer 0 tests.
     */
    {
        static const unsigned gpu_modes[] = {0u, 1u, 2u, 3u, 0x40u};
        static const unsigned sync_modes[] = {0u, 2u};
        const uint32_t chunks[] = {322u, 1612u, 214u, 322u, 1612u, 224u};
        const uint32_t total_cycles = 2148u * 2u + 10u;

        for (size_t g = 0u; g < sizeof(gpu_modes) / sizeof(gpu_modes[0]); ++g) {
            for (size_t s = 0u; s < sizeof(sync_modes) / sizeof(sync_modes[0]); ++s) {
                uint16_t bulk_counter = 0u;
                uint16_t chunk_counter = 0u;
                const uint16_t timer_mode = (uint16_t)(
                    PSX_TIMER_MODE_SYNC_ENABLE |
                    (sync_modes[s] << PSX_TIMER_MODE_SYNC_SHIFT));

                psx_machine_reset(&machine);
                assert(psx_gpu_gp1_write(&machine.gpu,
                    0x08000000u | gpu_modes[g]));
                assert(psx_timers_write16(&machine.timers,
                    PSX_TIMER_BASE_ADDR + 4u, timer_mode));
                assert(psx_machine_advance_system_cycles(&machine, total_cycles));
                assert(psx_timers_read16(&machine.timers,
                    PSX_TIMER_BASE_ADDR, &bulk_counter));

                psx_machine_reset(&machine);
                assert(psx_gpu_gp1_write(&machine.gpu,
                    0x08000000u | gpu_modes[g]));
                assert(psx_timers_write16(&machine.timers,
                    PSX_TIMER_BASE_ADDR + 4u, timer_mode));
                for (size_t i = 0u; i < sizeof(chunks) / sizeof(chunks[0]); ++i)
                    assert(psx_machine_advance_system_cycles(&machine, chunks[i]));
                assert(psx_timers_read16(&machine.timers,
                    PSX_TIMER_BASE_ADDR, &chunk_counter));
                assert(chunk_counter == bulk_counter);
                assert(machine.gpu_scanline_cycle_phase == 10u);
                assert(machine.timers.timer[0].sync_signal_active);
            }
        }
    }

    /* Timer 0 mode 3 starts on the first sampled HBlank level and
     * then free-runs; the initial raster position is already blanking.
     */
    {
        const uint16_t mode = (uint16_t)(
            PSX_TIMER_MODE_SYNC_ENABLE |
            (3u << PSX_TIMER_MODE_SYNC_SHIFT));

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u, mode));
        assert(psx_machine_advance_system_cycles(&machine, 5u));
        assert(machine.timers.timer[0].sync_started);
        assert(machine.timers.timer[0].counter == 5u);
        assert(psx_machine_advance_system_cycles(&machine, 3u));
        assert(machine.timers.timer[0].counter == 8u);
    }

    /* Timer 0 mode 1 must reset at each raster HBlank edge, after the
     * system clocks preceding that edge have elapsed. Ten cycles after the
     * second HBlank edge therefore leave 541 elapsed system ticks.
     */
    {
        const uint16_t mode = (uint16_t)(
            PSX_TIMER_MODE_SYNC_ENABLE |
            (1u << PSX_TIMER_MODE_SYNC_SHIFT));

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u, mode));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR, 77u));
        assert(psx_machine_advance_system_cycles(&machine,
                                                   2148u * 2u + 10u));
        assert(machine.timers.timer[0].sync_started);
        assert(machine.timers.timer[0].counter == 225u);
    }

    /* Timer 0 raster synchronization must be invariant to how the CPU
     * batches elapsed master-clock cycles around HBlank boundaries.
     */
    {
        const uint16_t mode = (uint16_t)(
            PSX_TIMER_MODE_SYNC_ENABLE |
            (1u << PSX_TIMER_MODE_SYNC_SHIFT));
        const uint32_t chunks[] = {1u, 2147u, 2u, 2148u, 8u};
        const uint32_t total_cycles = 2148u * 2u + 10u;
        uint16_t bulk_counter = 0u;
        uint16_t chunk_counter = 0u;
        uint16_t bulk_irq = 0u;
        uint32_t bulk_phase;
        uint32_t bulk_scanline;
        uint32_t bulk_frames;

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u, mode));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR, 77u));
        assert(psx_machine_advance_system_cycles(&machine, total_cycles));
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR, &bulk_counter));
        bulk_irq = psx_irq_read_stat(&machine.irq);
        bulk_phase = machine.gpu_scanline_cycle_phase;
        bulk_scanline = machine.gpu.display.video_scanline;
        bulk_frames = machine.gpu.display.video_frame_counter;

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u, mode));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR, 77u));
        for (size_t i = 0u; i < sizeof(chunks) / sizeof(chunks[0]); ++i)
            assert(psx_machine_advance_system_cycles(&machine, chunks[i]));
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR, &chunk_counter));
        assert(chunk_counter == bulk_counter);
        assert(psx_irq_read_stat(&machine.irq) == bulk_irq);
        assert(machine.gpu_scanline_cycle_phase == bulk_phase);
        assert(machine.gpu.display.video_scanline == bulk_scanline);
        assert(machine.gpu.display.video_frame_counter == bulk_frames);
        assert(machine.timers.timer[0].sync_started);
        assert(machine.timers.timer[0].sync_signal_active);
    }

    /* Timer 0 target IRQ/reset behavior must remain deterministic when
     * master-clock batches straddle HBlank boundaries. The target is reached
     * five clocks after each HBlank reset; after the second line and seven
     * more clocks, the counter must therefore read two and IRQ4 be pending.
     */
    {
        const uint16_t mode = (uint16_t)(
            PSX_TIMER_MODE_SYNC_ENABLE |
            (1u << PSX_TIMER_MODE_SYNC_SHIFT) |
            PSX_TIMER_MODE_RESET_AT_TARGET |
            PSX_TIMER_MODE_IRQ_TARGET);
        const uint32_t chunks[] = {2147u, 1u, 3u, 2145u, 5u, 2u};
        uint16_t bulk_counter = 0u;
        uint16_t chunk_counter = 0u;
        uint16_t bulk_irq = 0u;
        uint16_t bulk_mode;

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 8u, 5u));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u, mode));
        assert(psx_machine_advance_system_cycles(&machine, 2148u * 2u + 7u));
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR, &bulk_counter));
        bulk_irq = psx_irq_read_stat(&machine.irq);
        bulk_mode = machine.timers.timer[0].mode;
        assert(bulk_counter == 2u);
        assert((bulk_irq & (1u << 4u)) != 0u);
        assert((bulk_mode & PSX_TIMER_MODE_REACHED_TARGET) != 0u);

        psx_machine_reset(&machine);
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 8u, 5u));
        assert(psx_timers_write16(&machine.timers,
            PSX_TIMER_BASE_ADDR + 4u, mode));
        for (size_t i = 0u; i < sizeof(chunks) / sizeof(chunks[0]); ++i)
            assert(psx_machine_advance_system_cycles(&machine, chunks[i]));
        assert(psx_timers_read16(&machine.timers,
            PSX_TIMER_BASE_ADDR, &chunk_counter));
        assert(chunk_counter == bulk_counter);
        assert(psx_irq_read_stat(&machine.irq) == bulk_irq);
        assert(machine.timers.timer[0].mode == bulk_mode);
        assert(machine.timers.timer[0].irq_fired);
    }

    psx_machine_reset(&machine);
    /* SPU output cadence is clocked at 44.1 kHz: 768 master cycles/frame. */
    {
        int16_t frame[2] = {1, 1};
        assert(psx_machine_advance_system_cycles(&machine, 767u));
        assert(machine.spu_audio_cycle_phase == 767u);
        assert(machine.audio_output.frame_count == 0u);
        assert(psx_machine_advance_system_cycles(&machine, 1u));
        assert(machine.spu_audio_cycle_phase == 0u);
        assert(machine.audio_output.frame_count == 1u);
        assert(psx_machine_read_audio(&machine, frame, 1u) == 1u);
        assert(frame[0] == 0 && frame[1] == 0);

        psx_machine_reset(&machine);
        assert(psx_machine_advance_system_cycles(&machine, 1537u));
        assert(machine.audio_output.frame_count == 2u);
        assert(machine.spu_audio_cycle_phase == 1u);
        psx_machine_reset(&machine);
        assert(psx_machine_advance_system_cycles(&machine, 768u));
        assert(psx_machine_advance_system_cycles(&machine, 769u));
        assert(machine.audio_output.frame_count == 2u);
        assert(machine.spu_audio_cycle_phase == 1u);
    }
    psx_machine_reset(&machine);
    /* Machine-level SPU rendering feeds the bounded stereo output queue. */
    {
        int16_t output[PSX_AUDIO_BUFFER_CAPACITY_FRAMES * 2u] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
        size_t queued = 0u;
        assert(psx_machine_render_audio(&machine, 4u, &queued));
        assert(queued == 4u);
        assert(psx_machine_read_audio(&machine, output, 6u) == 4u);
        for (unsigned i = 0u; i < 12u; ++i) assert(output[i] == 0);
        assert(machine.audio_output.underrun_frames == 2u);
        assert(psx_machine_render_audio(
            &machine, PSX_AUDIO_BUFFER_CAPACITY_FRAMES + 8u, &queued));
        assert(queued == PSX_AUDIO_BUFFER_CAPACITY_FRAMES);
        assert(machine.audio_output.dropped_frames == 8u);
        assert(psx_machine_read_audio(&machine, output,
                                      PSX_AUDIO_BUFFER_CAPACITY_FRAMES) ==
               PSX_AUDIO_BUFFER_CAPACITY_FRAMES);
        psx_machine_reset(&machine);
        assert(machine.audio_output.frame_count == 0u);
        assert(machine.audio_output.dropped_frames == 0u);
    }
    /* End-to-end host path: machine SPU render -> machine PCM queue ->
     * KallistiOS-compatible byte-count callback, preserving stereo frames. */
    {
        PsxDreamcastAudioStream stream;
        int received = 0;
        int16_t *pcm;
        size_t queued = 0u;
        PsxSpuVoice *voice = &machine.spu.voices[0];

        psx_machine_reset(&machine);
        assert(psx_spu_write16(&machine.spu, PSX_SPU_BASE_ADDR,
                               0x3fffu));
        assert(psx_spu_write16(&machine.spu, PSX_SPU_BASE_ADDR + 0x002u,
                               0x3fffu));
        assert(psx_spu_write16(&machine.spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_MAIN_VOLUME_LEFT_OFFSET, 0x3fffu));
        assert(psx_spu_write16(&machine.spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_MAIN_VOLUME_RIGHT_OFFSET, 0x3fffu));
        voice->adsr2 = 0x1fc0u; /* Frozen sustain rate for deterministic PCM. */
        voice->envelope_level = PSX_SPU_ENVELOPE_MAX;
        voice->envelope_phase = PSX_SPU_ADSR_SUSTAIN;
        voice->current_sample = 12000;
        voice->current_sample_valid = true;
        machine.spu.active_voices = 1u;

        assert(psx_machine_render_audio(&machine, 4u, &queued));
        assert(queued == 4u);
        assert(psx_dreamcast_audio_stream_init(&stream,
                                               &machine.audio_output));
        pcm = (int16_t *)psx_dreamcast_audio_stream_callback(
            &stream, 4 * (int)sizeof(int16_t) * 2, &received);
        assert(pcm != NULL && received == 16);
        assert(pcm[0] > 0 && pcm[1] > 0);
        assert(pcm[2] == pcm[0] && pcm[3] == pcm[1]);
        assert(machine.audio_output.frame_count == 0u);
        assert(stream.silence_frames == 0u);
    }
    assert(psx_memory_write32(&machine.memory, 0u, 0xdecafbad));
    uint32_t memory_probe = 0u;
    assert(psx_memory_read32(&machine.memory, 0x80000000u, &memory_probe));
    assert(memory_probe == 0xdecafbad);

    /* Reset establishes a clean IRQ/COP0 state. */
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert(psx_irq_read_mask(&machine.irq) == 0u);
    assert(machine.cop0_status == 0u);
    assert(machine.cop0_cause == 0u);
    {
        uint32_t gte_value = 1u;
        assert(psx_gte_read_data(&machine.gte, 0u, &gte_value));
        assert(gte_value == 0u);
    }
    assert(!psx_machine_interrupt_eligible(&machine));

    /* Raise a source while masked: Cause.IP2 stays low. */
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* Enable the source and CPU mask/global enable. */
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));

    /* Disable CPU global enable: pending remains, delivery is inhibited. */
    machine.cop0_status &= ~PSX_COP0_STATUS_IEC;
    assert(!psx_machine_interrupt_eligible(&machine));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 2)) != 0u);

    /* Acknowledge source; refresh deasserts IP2. */
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* An eligible hardware IRQ is dispatched automatically at a safe
     * instruction boundary and redirects to the general exception vector.
     */
    machine.cpu.pc = 0x80000000u;
    machine.cpu.next_pc = 0x80000004u;
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x80000000u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_INTERRUPT << PSX_COP0_CAUSE_EXCCODE_SHIFT));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cpu.next_pc == 0x80000084u);
    assert((machine.cop0_status & PSX_COP0_STATUS_IEC) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));
    /* Clear the latched source before testing unrelated synchronous faults. */
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    /* Start the synchronous-fault suite from a clean CPU/COP0/IRQ state. */
    psx_machine_reset(&machine);

    /* XA PCM sample deadlines advance through the machine's shared clock. */
    {
        int16_t left[2] = {0, 0};
        int16_t right[2] = {0, 0};
        size_t frames = 0u;
        uint16_t rate = 0u;
        machine.cdrom.xa_audio_clock_rate_hz = 18900u;
        machine.cdrom.xa_pcm_sample_rate = 18900u;
        machine.cdrom.xa_pcm_count = 2u;
        machine.cdrom.xa_pcm_left[0] = 123;
        machine.cdrom.xa_pcm_right[0] = -123;
        machine.cdrom.xa_pcm_left[1] = 456;
        machine.cdrom.xa_pcm_right[1] = -456;
        assert(psx_machine_advance_system_cycles(&machine, 1791u));
        assert(machine.cdrom.xa_audio_due_frames == 0u);
        assert(psx_machine_advance_system_cycles(&machine, 1u));
        assert(machine.cdrom.xa_audio_due_frames == 1u);
        assert(psx_cdrom_read_xa_audio_timed(
            &machine.cdrom, left, right, 2u, &frames, &rate));
        assert(frames == 1u && rate == 18900u);
        assert(left[0] == 123 && right[0] == -123);
        assert(machine.cdrom.xa_pcm_count == 1u);
    }
    psx_machine_reset(&machine);

    /* Synchronous CPU faults are translated into architectural exceptions. */
    assert(psx_memory_write32(&machine.memory, 0x100u, 0x21280001u)); /* ADDI t0,t1,1 */
    machine.cpu.pc = 0x100u;
    machine.cpu.next_pc = 0x104u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x7fffffffu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x100u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_OVERFLOW << PSX_COP0_CAUSE_EXCCODE_SHIFT));
    assert(machine.cpu.pc == 0x80000080u);

    /* SYSCALL and BREAK use distinct architectural exception codes. */
    assert(psx_memory_write32(&machine.memory, 0x110u, 0x0000000cu));
    machine.cpu.pc = 0x110u;
    machine.cpu.next_pc = 0x114u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x110u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_SYSCALL << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    assert(psx_memory_write32(&machine.memory, 0x114u, 0x0000000du));
    machine.cpu.pc = 0x114u;
    machine.cpu.next_pc = 0x118u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x114u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_BREAKPOINT << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* Invalid fixed fields in an otherwise known SPECIAL opcode are reserved. */
    assert(psx_memory_write32(&machine.memory, 0x118u, 0x00284000u)); /* malformed SLL: rs != 0 */
    machine.cpu.pc = 0x118u;
    machine.cpu.next_pc = 0x11cu;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_RESERVED_INSTRUCTION <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A reserved instruction is no longer a silent boolean failure. */
    assert(psx_memory_write32(&machine.memory, 0x120u, 0xffffffffu));
    machine.cpu.pc = 0x120u;
    machine.cpu.next_pc = 0x124u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x120u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_RESERVED_INSTRUCTION << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A data bus fault records the effective address in BadVAddr. */
    assert(psx_memory_write32(&machine.memory, 0x140u, 0x8d280000u)); /* LW t0,0(t1) */
    machine.cpu.pc = 0x140u;
    machine.cpu.next_pc = 0x144u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x1f000000u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_BUS_DATA << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* Misaligned word loads produce Address Error Load, not Bus Error. */
    machine.cpu.pc = 0x140u;
    machine.cpu.next_pc = 0x144u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x181u);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x181u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_ADDRESS_LOAD << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* COP0 access in user mode produces Coprocessor Unusable. */
    assert(psx_memory_write32(&machine.memory, 0x180u, 0x40096000u)); /* MFC0 t1,Status */
    machine.cpu.pc = 0x180u;
    machine.cpu.next_pc = 0x184u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = PSX_COP0_STATUS_KUC;
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_COPROCESSOR_UNUSABLE << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* Overflow in a branch delay slot must report EPC=branch PC and Cause.BD. */
    assert(psx_memory_write32(&machine.memory, 0x160u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x164u, 0x21280001u)); /* ADDI delay slot */
    machine.cpu.pc = 0x160u;
    machine.cpu.next_pc = 0x164u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x7fffffffu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x160u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_OVERFLOW << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A REGIMM control transfer in a branch delay slot is rejected and
     * retains the original branch PC in EPC with Cause.BD set.
     */
    assert(psx_memory_write32(&machine.memory, 0x190u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x194u,
                              (0x01u << 26) | (8u << 21) | 1u)); /* BLTZ t0,+1 */
    machine.cpu.pc = 0x190u;
    machine.cpu.next_pc = 0x194u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xffffffffu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x190u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_RESERVED_INSTRUCTION <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A pending load is retired before a synchronous exception enters
     * the handler; the faulting instruction itself is not retired.
     */
    assert(psx_memory_write32(&machine.memory, 0x300u, 0xa5a55a5au));
    assert(psx_memory_write32(&machine.memory, 0x1a0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1a4u, 0x0000000cu)); /* SYSCALL */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x11111111u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x300u);
    machine.cpu.pc = 0x1a0u;
    machine.cpu.next_pc = 0x1a4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x11111111u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xa5a55a5au);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1a4u);

    /* A data-bus fault immediately after a load must retire the older
     * load, while leaving the faulting load's destination untouched.
     */
    assert(psx_memory_write32(&machine.memory, 0x304u, 0x13579bdfu));
    assert(psx_memory_write32(&machine.memory, 0x1b0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1b4u, 0x8d6a0000u)); /* LW t2,0(t3) */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x11111111u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x304u);
    psx_cpu_write_gpr(&machine.cpu, 10u, 0x22222222u);
    psx_cpu_write_gpr(&machine.cpu, 11u, 0x1f000000u);
    machine.cpu.pc = 0x1b0u;
    machine.cpu.next_pc = 0x1b4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x11111111u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x13579bdfu);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x22222222u);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1b4u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_BUS_DATA <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A hardware interrupt at the instruction boundary also retires
     * the pending load before entering the exception handler.
     */
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);
    assert(psx_memory_write32(&machine.memory, 0x308u, 0x2468ace0u));
    assert(psx_memory_write32(&machine.memory, 0x1c0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1c4u, 0x240c0001u)); /* ADDIU t4,zero,1 */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x33333333u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x308u);
    machine.cpu.pc = 0x1c0u;
    machine.cpu.next_pc = 0x1c4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x33333333u);
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x2468ace0u);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1c4u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_INTERRUPT <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));
    /* Acknowledge the source so later tests start with a clean controller. */
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);

    /* An IRQ must wait until a branch delay-slot load has executed.
     * Once the slot completes, the pending load retires before IRQ entry;
     * EPC then identifies the branch target, not the delay-slot address.
     */
    assert(psx_memory_write32(&machine.memory, 0x30cu, 0xcafebabeu));
    assert(psx_memory_write32(&machine.memory, 0x1d0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x1d4u, 0x8d280000u)); /* LW t0,0(t1), delay slot */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x44444444u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x30cu);
    machine.cpu.pc = 0x1d0u;
    machine.cpu.next_pc = 0x1d4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    assert(psx_machine_step(&machine)); /* IRQ deferred; execute delay slot */
    assert(!machine.cpu.delay_slot_pending);
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x44444444u);
    assert(psx_machine_step(&machine)); /* IRQ now accepted */
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xcafebabeu);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1d8u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_INTERRUPT <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);

    /* A synchronous overflow immediately after a delay-slot LW must
     * retire that load, report EPC at the branch target, and clear Cause.BD.
     */
    assert(psx_memory_write32(&machine.memory, 0x310u, 0xfacec0deu));
    assert(psx_memory_write32(&machine.memory, 0x1e0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x1e4u, 0x8d480000u)); /* LW t0,0(t2), delay slot */
    assert(psx_memory_write32(&machine.memory, 0x1e8u, 0x21280001u)); /* ADDI t0,t1,1 */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x55555555u);
    psx_cpu_write_gpr(&machine.cpu, 10u, 0x310u);
    machine.cpu.pc = 0x1e0u;
    machine.cpu.next_pc = 0x1e4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x7fffffffu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine)); /* branch */
    assert(psx_machine_step(&machine)); /* delay-slot load */
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x55555555u);
    assert(psx_machine_step(&machine)); /* target ADDI overflows */
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xfacec0deu);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1e8u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_OVERFLOW <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* MFC0 is a delayed GPR load: the immediately following ADDIU reads
     * the old destination value, then the COP0 result becomes visible.
     */
    assert(psx_memory_write32(&machine.memory, 0x200u, 0x40086000u)); /* MFC0 t0,Status */
    assert(psx_memory_write32(&machine.memory, 0x204u, 0x25090001u)); /* ADDIU t1,t0,1 */
    machine.cop0_status = 0x00001234u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 5u);
    machine.cpu.pc = 0x200u;
    machine.cpu.next_pc = 0x204u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 5u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x1234u);
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 6u);

    /* MTC0 reads the old GPR value when it immediately follows LW. The load
     * retires after the COP0 write, so EPC receives the pre-load value.
     */
    assert(psx_memory_write32(&machine.memory, 0x320u, 0xabcdef01u));
    assert(psx_memory_write32(&machine.memory, 0x210u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x214u, 0x40887000u)); /* MTC0 t0,EPC */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x11112222u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x320u);
    machine.cpu.pc = 0x210u;
    machine.cpu.next_pc = 0x214u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x11112222u);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xabcdef01u);
    assert(!machine.cpu.load_delay_pending);

    /* An IRQ between MFC0 and its consumer must retire the delayed COP0
     * transfer before exception entry, just like a memory load.
     */
    machine.cop0_epc = 0x13572468u;
    assert(psx_memory_write32(&machine.memory, 0x218u, 0x40087000u)); /* MFC0 t0,EPC */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xdeadbeefu);
    machine.cpu.pc = 0x218u;
    machine.cpu.next_pc = 0x21cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xdeadbeefu);
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x13572468u);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x21cu);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_INTERRUPT <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);

    /* Halfword loads/stores and word stores report alignment faults with
     * the original effective address in BadVAddr.
     */
    assert(psx_memory_write32(&machine.memory, 0x340u,
                              (0x21u << 26) | (9u << 21) | (8u << 16)));
    machine.cpu.pc = 0x340u;
    machine.cpu.next_pc = 0x344u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x401u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x401u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_ADDRESS_LOAD <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    assert(psx_memory_write32(&machine.memory, 0x344u,
                              (0x29u << 26) | (9u << 21) | (8u << 16)));
    machine.cpu.pc = 0x344u;
    machine.cpu.next_pc = 0x348u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x401u);
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xbeefu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x401u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_ADDRESS_STORE <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    assert(psx_memory_write32(&machine.memory, 0x348u,
                              (0x2bu << 26) | (9u << 21) | (8u << 16)));
    machine.cpu.pc = 0x348u;
    machine.cpu.next_pc = 0x34cu;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x402u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x402u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_ADDRESS_STORE <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* An aligned store to an unmapped device region is a Bus Data exception,
     * not an Address Error Store.
     */
    assert(psx_memory_write32(&machine.memory, 0x34cu,
                              (0x2bu << 26) | (9u << 21) | (8u << 16)));
    machine.cpu.pc = 0x34cu;
    machine.cpu.next_pc = 0x350u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x1f000000u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_BUS_DATA <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* R3000A trap instructions raise exception code 13 only when true. */
    assert(psx_memory_write32(&machine.memory, 0x350u,
                              (8u << 21) | (9u << 16) | 0x34u)); /* TEQ t0,t1 */
    machine.cpu.pc = 0x350u;
    machine.cpu.next_pc = 0x354u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x12345678u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x12345678u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_TRAP << PSX_COP0_CAUSE_EXCCODE_SHIFT));
    assert(machine.cop0_epc == 0x350u);

    /* A false TNEI condition does not trap and advances normally. */
    assert(psx_memory_write32(&machine.memory, 0x354u,
                              (0x01u << 26) | (8u << 21) | (14u << 16) | 7u));
    machine.cpu.pc = 0x354u;
    machine.cpu.next_pc = 0x358u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 7u);
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x358u);

    /* Signed TLTI compares against the sign-extended immediate. */
    assert(psx_memory_write32(&machine.memory, 0x358u,
                              (0x01u << 26) | (8u << 21) | (10u << 16) | 0xffffu));
    machine.cpu.pc = 0x358u;
    machine.cpu.next_pc = 0x35cu;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xfffffffeu); /* -2 */
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_TRAP << PSX_COP0_CAUSE_EXCCODE_SHIFT));
    assert(machine.cop0_epc == 0x358u);

    /* A true conditional trap in a branch delay slot must report the
     * branch address in EPC and set Cause.BD.
     */
    assert(psx_memory_write32(&machine.memory, 0x360u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x364u,
                              (8u << 21) | (9u << 16) | 0x34u)); /* TEQ t0,t1 */
    machine.cpu.pc = 0x360u;
    machine.cpu.next_pc = 0x364u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x55u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x55u);
    assert(psx_machine_step(&machine)); /* branch */
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine)); /* trap in delay slot */
    assert(machine.cop0_epc == 0x360u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_TRAP << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A false trap in a delay slot retires normally and preserves the
     * already-selected branch target without manufacturing another slot.
     */
    assert(psx_memory_write32(&machine.memory, 0x368u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x36cu,
                              (8u << 21) | (9u << 16) | 0x34u)); /* TEQ t0,t1 */
    machine.cpu.pc = 0x368u;
    machine.cpu.next_pc = 0x36cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    machine.cop0_cause = 0u; /* Do not inherit the previous trap's ExcCode. */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x55u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0xaau);
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x370u);
    assert(!machine.cpu.delay_slot_pending);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) !=
           ((uint32_t)PSX_EXCEPTION_TRAP << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    free(gpu_vram);
    free(bios);
    free(ram);
    puts("DreamPSX machine-state tests: OK");
    return 0;
}
