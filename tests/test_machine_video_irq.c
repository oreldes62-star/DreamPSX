#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dreampsx/cop0.h"
#include "dreampsx/cop0_irq.h"
#include "dreampsx/machine.h"

int main(void)
{
    PsxMachine machine;
    PsxGpuVideoState video;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    uint16_t *vram = (uint16_t *)calloc(
        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT, sizeof(uint16_t));
    assert(ram && bios && vram);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE,
                            bios, PSX_BIOS_SIZE, vram,
                            PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* GP1(07h): visible lines [16, 256). Start in NTSC progressive mode. */
    assert(psx_gpu_gp1_write(&machine.gpu,
        0x07000000u | (256u << 10u) | 16u));
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
    psx_irq_write_mask(&machine.irq, (1u << 0u) | (1u << 5u));

    /* Timer1 selects HBlank (clock source 1), targets its first line tick,
     * and raises IRQ source 5 through the machine adapter. */
    assert(psx_timers_write16(&machine.timers, 0x1f801118u, 1u));
    assert(psx_timers_write16(&machine.timers, 0x1f801114u,
                              (uint16_t)((1u << 8u) | (1u << 4u))));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;

    assert(psx_machine_video_advance_scanline(&machine, &video));
    assert(video.vblank && video.vblank_started);
    assert(video.hblank_pulse);
    assert(machine.timers.timer[1].counter == 1u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 5u)) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert((psx_irq_read_stat(&machine.irq) & 1u) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));

    /* Acknowledge the one-shot Timer1 IRQ. Further HBlank pulses still
     * advance Timer1 but do not retrigger its one-shot interrupt. */
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 5u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 5u)) == 0u);
    assert(machine.timers.timer[1].counter == 1u);
    /* Clearing Timer 1 must not erase the independent VBlank request.
     * COP0 IP2 therefore remains asserted while IRQ0 is still pending. */
    assert((psx_irq_read_stat(&machine.irq) & 1u) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));
    assert(psx_machine_timer_advance_clock(&machine, 1u,
                                            PSX_TIMER_CLOCK_HBLANK, 1u));
    assert(machine.timers.timer[1].counter == 2u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 5u)) == 0u);
    assert((psx_irq_read_stat(&machine.irq) & 1u) != 0u);

    /* IRQ0 remains level-asserted throughout VBlank, while I_STAT latches
     * the request. Entering the visible range deasserts the source line.
     */
    for (unsigned i = 0u; i < 15u; ++i)
        assert(psx_machine_video_advance_scanline(&machine, &video));
    assert(video.scanline == 16u && !video.vblank && video.vblank_ended);
    assert((machine.irq.source_level & 1u) == 0u);
    assert((psx_irq_read_stat(&machine.irq) & 1u) != 0u);

    /* Acknowledge the latched request after the source has gone low. */
    psx_irq_write_stat(&machine.irq, (uint16_t)~1u);
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & 1u) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    /* The next VBlank edge is latched again, proving edge re-arming. */
    for (unsigned i = 0u; i < 240u; ++i)
        assert(psx_machine_video_advance_scanline(&machine, &video));
    assert(video.scanline == 256u && video.vblank_started);
    assert((psx_irq_read_stat(&machine.irq) & 1u) != 0u);

    assert(!psx_machine_video_advance_scanline(NULL, &video));
    assert(!psx_machine_video_advance_scanline(&machine, NULL));

    /* CPU/master-clock cycles now drive the GPU raster automatically.
     * Splitting one scanline across calls must preserve its fractional phase.
     */
    psx_gpu_reset(&machine.gpu);
    machine.gpu_scanline_cycle_phase = 0u;
    assert(psx_gpu_gp1_write(&machine.gpu,
        0x07000000u | (256u << 10u) | 16u));
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
    assert(psx_machine_advance_system_cycles(&machine, 2152u));
    assert(machine.gpu.display.video_scanline == 0u);
    assert(machine.gpu_scanline_cycle_phase == 2152u);
    assert(psx_machine_advance_system_cycles(&machine, 1u));
    assert(machine.gpu.display.video_scanline == 1u);
    assert(machine.gpu_scanline_cycle_phase == 0u);
    assert(machine.gpu.display.video_vblank);

    /* GP1(08h) changes output mode without restarting the physical PAL
     * raster: the 1000-cycle position within the current line is retained.
     */
    psx_gpu_reset(&machine.gpu);
    machine.gpu_scanline_cycle_phase = 0u;
    assert(psx_machine_set_video_clock_region(
        &machine, PSX_VIDEO_CLOCK_PAL));
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
    assert(psx_machine_advance_system_cycles(&machine, 1000u));
    assert(machine.gpu_scanline_cycle_phase == 1000u);
    assert(psx_gpu_gp1_write(&machine.gpu,
        0x07000000u | (256u << 10u) | 16u));
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000008u));
    assert(psx_machine_advance_system_cycles(&machine, 1168u));
    assert(machine.gpu.display.video_scanline == 0u);
    assert(machine.gpu_scanline_cycle_phase == 2168u);
    assert(psx_machine_advance_system_cycles(&machine, 1u));
    assert(machine.gpu.display.video_scanline == 1u);
    assert(machine.gpu_scanline_cycle_phase == 0u);

    assert(psx_machine_set_video_clock_region(
        &machine, PSX_VIDEO_CLOCK_NTSC));

    /* Raster advancement must be independent of how the caller batches
     * master-clock cycles, including intervals spanning many scanlines.
     */
    {
        const uint32_t total_cycles = 2153u * 1000u + 123u;
        unsigned expected_scanline;
        uint32_t expected_phase;

        psx_gpu_reset(&machine.gpu);
        assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
        machine.gpu_scanline_cycle_phase = 0u;
        machine.gpu_scanline_cycle_remainder = 0u;
        assert(psx_machine_advance_system_cycles(&machine, total_cycles));
        expected_scanline = machine.gpu.display.video_scanline;
        expected_phase = machine.gpu_scanline_cycle_phase;

        psx_gpu_reset(&machine.gpu);
        machine.gpu_scanline_cycle_phase = 0u;
        machine.gpu_scanline_cycle_remainder = 0u;
        assert(psx_machine_advance_system_cycles(&machine, 2153u * 333u + 1u));
        assert(psx_machine_advance_system_cycles(&machine, 2153u * 400u + 77u));
        assert(psx_machine_advance_system_cycles(
            &machine, total_cycles - (2153u * 333u + 1u) -
                      (2153u * 400u + 77u)));
        assert(machine.gpu.display.video_scanline == expected_scanline);
        assert(machine.gpu_scanline_cycle_phase == expected_phase);
    }

    /* VBlank IRQ state must be independent of cycle batching in both
     * physical oscillator regions, including a real latched IRQ0 event.
     */
    for (unsigned region = 0u; region < 2u; ++region) {
        const uint32_t line_cycles = region == 0u ? 2153u : 2169u;
        const uint32_t total_cycles = line_cycles * 600u + 317u;
        uint16_t whole_irq_stat;
        uint16_t whole_timer1;
        uint32_t whole_source_level;
        uint32_t whole_cause;
        uint16_t whole_scanline;
        uint32_t whole_phase;

        assert(psx_machine_set_video_clock_region(
            &machine, region == 0u ?
            PSX_VIDEO_CLOCK_NTSC : PSX_VIDEO_CLOCK_PAL));
        psx_machine_reset(&machine);
        assert(psx_gpu_gp1_write(&machine.gpu,
            0x07000000u | (256u << 10u) | 16u));
        assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
        psx_irq_write_mask(&machine.irq, 1u);
        machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
        assert(psx_machine_advance_system_cycles(&machine, total_cycles));
        whole_irq_stat = psx_irq_read_stat(&machine.irq);
        assert((whole_irq_stat & 1u) != 0u);
        whole_timer1 = machine.timers.timer[1].counter;
        whole_source_level = machine.irq.source_level;
        whole_cause = machine.cop0_cause;
        whole_scanline = machine.gpu.display.video_scanline;
        whole_phase = machine.gpu_scanline_cycle_phase;

        psx_machine_reset(&machine);
        assert(psx_gpu_gp1_write(&machine.gpu,
            0x07000000u | (256u << 10u) | 16u));
        assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
        psx_irq_write_mask(&machine.irq, 1u);
        machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
        {
            const uint32_t chunks[] = {
                1u, 322u, line_cycles, 4097u, line_cycles * 200u, 17u,
                total_cycles - 1u - 322u - line_cycles - 4097u -
                    line_cycles * 200u - 17u
            };
            for (size_t i = 0u; i < sizeof(chunks) / sizeof(chunks[0]); ++i)
                assert(psx_machine_advance_system_cycles(&machine, chunks[i]));
        }
        assert(psx_irq_read_stat(&machine.irq) == whole_irq_stat);
        assert(machine.timers.timer[1].counter == whole_timer1);
        assert(machine.irq.source_level == whole_source_level);
        assert(machine.cop0_cause == whole_cause);
        assert(machine.gpu.display.video_scanline == whole_scanline);
        assert(machine.gpu_scanline_cycle_phase == whole_phase);
    }

    /* Interlaced 480-line NTSC output alternates 262/263-line fields.
     * A displayed frame completes only after the odd field returns to even.
     */
    psx_gpu_reset(&machine.gpu);
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000024u));
    assert(machine.gpu.display.video_frame_counter == 0u);
    for (unsigned i = 0u; i < 262u; ++i)
        assert(psx_gpu_video_advance_scanline(&machine.gpu, &video));
    assert(video.field_started && video.field_odd);
    assert(video.lines_per_field == 263u);
    assert(video.frame_counter == 0u);
    for (unsigned i = 0u; i < 262u; ++i)
        assert(psx_gpu_video_advance_scanline(&machine.gpu, &video));
    assert(!video.field_started && video.field_odd);
    assert(video.frame_counter == 0u);
    assert(psx_gpu_video_advance_scanline(&machine.gpu, &video));
    assert(video.field_started && !video.field_odd);
    assert(video.frame_started && video.frame_counter == 1u);
    assert(video.lines_per_field == 262u);

    /* PAL interlace uses 312/313-line fields with the same full-frame rule. */
    psx_gpu_reset(&machine.gpu);
    assert(psx_gpu_gp1_write(&machine.gpu, 0x0800002cu));
    for (unsigned i = 0u; i < 312u; ++i)
        assert(psx_gpu_video_advance_scanline(&machine.gpu, &video));
    assert(video.field_started && video.field_odd);
    assert(video.lines_per_field == 313u);
    assert(video.frame_counter == 0u);
    for (unsigned i = 0u; i < 313u; ++i)
        assert(psx_gpu_video_advance_scanline(&machine.gpu, &video));
    assert(video.field_started && !video.field_odd);
    assert(video.frame_started && video.frame_counter == 1u);
    assert(video.lines_per_field == 312u);

    /* GPU command IRQ1 reaches COP0 IP2 and is accepted at an instruction
     * boundary. Device acknowledge drops the source; I_STAT is separate. */
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 1u));
    machine.cop0_status = (1u << 0u) | (1u << 10u); /* IEc + IM2 */
    machine.cpu.pc = 0x100u;
    machine.cpu.next_pc = 0x104u;
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0x1f000000u));
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) == 0u);
    assert(machine.cop0_epc == 0x100u);
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x02000000u));
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 1u));
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* A VBlank IRQ arriving between a taken branch and its delay slot
     * must be deferred until the delay-slot instruction has retired. */
    psx_machine_reset(&machine);
    assert(psx_gpu_gp1_write(&machine.gpu,
        0x07000000u | (256u << 10u) | 16u));
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
    psx_irq_write_mask(&machine.irq, 1u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x24090007u)); /* ADDIU t1,zero,7 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x240a0009u)); /* ADDIU t2,zero,9 */
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(machine.cpu.pc == 4u && machine.cpu.next_pc == 8u);
    assert(psx_machine_video_advance_scanline(&machine, &video));
    assert(video.vblank && video.vblank_started);
    assert((psx_irq_read_stat(&machine.irq) & 1u) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_step(&machine)); /* VBlank waits for the delay slot */
    assert(!machine.cpu.delay_slot_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 7u);
    assert(machine.cpu.pc == 8u && machine.cpu.next_pc == 12u);
    assert(psx_machine_step(&machine)); /* Accept IRQ at next boundary */
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 8u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) == 0u);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0u);

    /* GPU command IRQ1 must obey the same branch-delay rule as VBlank
     * and timer interrupts, even though its source is the GP0 command port. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 1u));
    assert(psx_bus_write32(&machine.bus, 0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x24090007u)); /* ADDIU t1,zero,7 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x240a0009u)); /* ADDIU t2,zero,9 */
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0x1f000000u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 1u)) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_step(&machine)); /* GPU IRQ1 waits for delay slot */
    assert(!machine.cpu.delay_slot_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 7u);
    assert(machine.cpu.pc == 8u && machine.cpu.next_pc == 12u);
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 8u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) == 0u);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0u);
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x02000000u));
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 1u));
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    free(vram);
    free(bios);
    free(ram);
    puts("DreamPSX machine video IRQ tests passed");
    return 0;
}
