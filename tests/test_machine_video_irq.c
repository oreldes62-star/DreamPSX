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

    /* Simultaneous VBlank IRQ0 and GPU command IRQ1 requests must remain
     * independently latched when either device is acknowledged. */
    psx_machine_reset(&machine);
    assert(psx_gpu_gp1_write(&machine.gpu,
        0x07000000u | (256u << 10u) | 16u));
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000000u));
    psx_irq_write_mask(&machine.irq, (uint16_t)((1u << 0u) | (1u << 1u)));
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_machine_video_advance_scanline(&machine, &video));
    assert(video.vblank_started);
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0x1f000000u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & 0x0003u) == 0x0003u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* Acknowledge the GPU's device-side IRQ1, then clear only its I_STAT bit.
     * VBlank IRQ0 must remain pending on the shared CPU interrupt line. */
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x02000000u));
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 1u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & 0x0003u) == 0x0001u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* Once VBlank ends, its source line falls; clearing IRQ0 now drops IP2. */
    for (unsigned i = 0u; i < 15u; ++i)
        assert(psx_machine_video_advance_scanline(&machine, &video));
    assert(video.scanline == 16u && !video.vblank);
    psx_irq_write_stat(&machine.irq, (uint16_t)~1u);
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & 0x0003u) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    /* A timer IRQ and a GPU command IRQ share COP0 IP2 but retain
     * independent I_STAT latches when either peripheral is acknowledged. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq,
        (uint16_t)((1u << 1u) | (1u << 4u)));
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_timers_write16(&machine.timers,
        0x1f801108u, 1u));
    assert(psx_timers_write16(&machine.timers,
        0x1f801104u,
        (uint16_t)(PSX_TIMER_MODE_IRQ_TARGET |
                   PSX_TIMER_MODE_IRQ_REPEAT)));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0x1f000000u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) &
            ((1u << 1u) | (1u << 4u))) ==
           ((1u << 1u) | (1u << 4u)));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));

    /* Clearing GPU IRQ1 cannot erase Timer0's independent request. */
    assert(psx_bus_write32(&machine.bus, 0x1f801814u, 0x02000000u));
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 1u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 1u)) == 0u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));

    /* Clearing the final timer request is the only operation that drops IP2. */
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 4u));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    /* A guest SW to I_STAT must update the shared COP0 IP2 line at the
     * end of that instruction, without clearing unrelated IRQ sources.
     * Both source lines fall first, leaving independent latched requests. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, 0x0003u);
    assert(psx_irq_set_line(&machine.irq, 0u, true));
    assert(psx_irq_set_line(&machine.irq, 1u, true));
    assert(psx_irq_set_line(&machine.irq, 0u, false));
    assert(psx_irq_set_line(&machine.irq, 1u, false));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & 0x0003u) == 0x0003u);

    /* t0 = 0xfffffffd (acknowledge IRQ1 only); t1 = I_STAT address. */
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c08ffffu)); /* LUI t0,ffff */
    assert(psx_bus_write32(&machine.bus, 4u, 0x3508fffdu)); /* ORI t0,t0,fffd */
    assert(psx_bus_write32(&machine.bus, 8u, 0x3c091f80u)); /* LUI t1,1f80 */
    assert(psx_bus_write32(&machine.bus, 12u, 0x35291070u)); /* ORI t1,t1,1070 */
    assert(psx_bus_write32(&machine.bus, 16u, 0xad280000u)); /* SW t0,0(t1) */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u; /* Prepare registers before enabling IRQs. */
    for (unsigned i = 0u; i < 4u; ++i)
        assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xfffffffdu);
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0x1f801070u);
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    psx_machine_refresh_interrupts(&machine);
    assert(psx_machine_interrupt_eligible(&machine));
    assert(psx_machine_step(&machine)); /* Guest acknowledges IRQ1 via SW. */
    assert((psx_irq_read_stat(&machine.irq) & 0x0003u) == 0x0001u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));
    assert(machine.cpu.pc == 20u);

    /* A second guest SW clears the final latched source. COP0 IP2 must
     * fall at this instruction boundary, not one instruction later. */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xfffffffeu); /* acknowledge IRQ0 */
    assert(psx_bus_write32(&machine.bus, 20u, 0xad280000u)); /* SW t0,0(t1) */
    assert(psx_machine_step(&machine));
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));
    assert(machine.cpu.pc == 24u);

    /* A Timer0 event occurring in the CPU instruction's elapsed cycles is
     * sampled only after that instruction retires, then accepted at the
     * following instruction boundary. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 4u));
    assert(psx_timers_write16(&machine.timers, 0x1f801108u, 1u));
    assert(psx_timers_write16(&machine.timers, 0x1f801104u,
        (uint16_t)(PSX_TIMER_MODE_IRQ_TARGET | PSX_TIMER_MODE_IRQ_REPEAT)));
    assert(psx_bus_write32(&machine.bus, 0u, 0x240b002au)); /* ADDIU t3,zero,42 */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 42u);
    assert(machine.cpu.pc == 4u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));
    assert(psx_machine_step(&machine)); /* IRQ accepted at next boundary */
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 4u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) == 0u);
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 42u);


    /* A guest SW to Timer0 MODE must reset the toggle output line while
     * preserving the already-latched I_STAT request. This checks the full
     * CPU -> bus -> timer/IRQ path, not only direct register helpers. */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR,
                           (uint16_t)(1u << 4u)));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_RESET_AT_TARGET |
                   PSX_TIMER_MODE_IRQ_TARGET | PSX_TIMER_MODE_IRQ_REPEAT |
                   PSX_TIMER_MODE_IRQ_TOGGLE)));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((machine.irq.source_level & (1u << 4u)) != 0u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);

    /* t0 = 0x1f801104 (Timer0 MODE), t1 = 0. */
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081104u)); /* ORI t0,t0,1104 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x24090000u)); /* ADDIU t1,zero,0 */
    assert(psx_bus_write32(&machine.bus, 12u, 0xad090000u)); /* SW t1,0(t0) */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u; /* Let the guest finish the MODE write first. */
    for (unsigned i = 0u; i < 4u; ++i)
        assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 16u);
    assert((machine.irq.source_level & (1u << 4u)) == 0u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    assert(!machine.timers.timer[0].irq_toggle_state);
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));


    /* Guest LW accesses to COUNT and TARGET must return their own
     * registers through the same MMIO path. COUNT is sampled after the
     * two address-setup instructions have each advanced the timer once. */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR, 0x1200u));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 0x3456u));
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081100u)); /* ORI t0,t0,1100 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x8d090000u)); /* LW t1,COUNT */
    assert(psx_bus_write32(&machine.bus, 12u, 0x00000000u)); /* NOP */
    assert(psx_bus_write32(&machine.bus, 16u, 0x25080008u)); /* ADDIU t0,t0,8 */
    assert(psx_bus_write32(&machine.bus, 20u, 0x8d0a0000u)); /* LW t2,TARGET */
    assert(psx_bus_write32(&machine.bus, 24u, 0x00000000u)); /* NOP */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    for (unsigned i = 0u; i < 7u; ++i)
        assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0x1202u);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x3456u);

    /* Guest stores must program COUNT and TARGET through the CPU bus,
     * and subsequent guest loads must observe the written values. */
    psx_machine_reset(&machine);
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u));  /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081100u));  /* ORI t0,t0,1100 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x24092222u));  /* ADDIU t1,zero,2222 */
    assert(psx_bus_write32(&machine.bus, 12u, 0xad090000u)); /* SW t1,COUNT */
    assert(psx_bus_write32(&machine.bus, 16u, 0x24093333u)); /* ADDIU t1,zero,3333 */
    assert(psx_bus_write32(&machine.bus, 20u, 0xad090008u)); /* SW t1,TARGET */
    assert(psx_bus_write32(&machine.bus, 24u, 0x8d0a0000u)); /* LW t2,COUNT */
    assert(psx_bus_write32(&machine.bus, 28u, 0x00000000u)); /* NOP */
    assert(psx_bus_write32(&machine.bus, 32u, 0x25080008u)); /* ADDIU t0,t0,8 */
    assert(psx_bus_write32(&machine.bus, 36u, 0x8d0b0000u)); /* LW t3,TARGET */
    assert(psx_bus_write32(&machine.bus, 40u, 0x00000000u)); /* NOP */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    for (unsigned i = 0u; i < 11u; ++i)
        assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x2225u);
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 0x3333u);

    /* A guest LW of Timer0 MODE must observe the latched reached-target
     * flag, then clear that read-to-clear flag in the peripheral. */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((machine.timers.timer[0].mode &
            PSX_TIMER_MODE_REACHED_TARGET) != 0u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081104u)); /* ORI t0,t0,1104 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x8d090000u)); /* LW t1,0(t0) */
    assert(psx_bus_write32(&machine.bus, 12u, 0x00000000u)); /* NOP: load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    for (unsigned i = 0u; i < 4u; ++i)
        assert(psx_machine_step(&machine));
    assert((psx_cpu_read_gpr(&machine.cpu, 9u) &
            PSX_TIMER_MODE_REACHED_TARGET) != 0u);
    assert((machine.timers.timer[0].mode &
            PSX_TIMER_MODE_REACHED_TARGET) == 0u);

    /* The same guest read path must clear OVERFLOW independently. */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR, 0xffffu));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 0x1234u));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((machine.timers.timer[0].mode &
            PSX_TIMER_MODE_REACHED_OVERFLOW) != 0u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081104u)); /* ORI t0,t0,1104 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x8d090000u)); /* LW t1,0(t0) */
    assert(psx_bus_write32(&machine.bus, 12u, 0x00000000u)); /* NOP */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    for (unsigned i = 0u; i < 4u; ++i)
        assert(psx_machine_step(&machine));
    assert((psx_cpu_read_gpr(&machine.cpu, 9u) &
            PSX_TIMER_MODE_REACHED_OVERFLOW) != 0u);
    assert((machine.timers.timer[0].mode &
            PSX_TIMER_MODE_REACHED_OVERFLOW) == 0u);

    /* All three timer IRQ sources must latch independently when their
     * distinct clocks run together during one NTSC scanline:
     * Timer 0 DOT, Timer 1 HBlank, Timer 2 SYSTEM/8. */
    psx_machine_reset(&machine);
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000001u));
    psx_irq_write_mask(&machine.irq,
        (uint16_t)((1u << 4u) | (1u << 5u) | (1u << 6u)));
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    for (unsigned timer = 0u; timer < 3u; ++timer) {
        const uint32_t base = PSX_TIMER_BASE_ADDR +
                              timer * PSX_TIMER_STRIDE;
        assert(psx_timers_write16(&machine.timers, base + 8u, 1u));
        assert(psx_timers_write16(&machine.timers, base + 4u,
            (uint16_t)(1u << PSX_TIMER_MODE_CLOCK_SHIFT |
                       PSX_TIMER_MODE_IRQ_TARGET |
                       PSX_TIMER_MODE_IRQ_REPEAT)));
    }
    assert(psx_machine_advance_system_cycles(&machine, 2153u));
    assert(machine.timers.timer[0].counter != 0u);
    assert(machine.timers.timer[1].counter == 1u);
    assert(machine.timers.timer[2].counter == 2153u / 8u);
    assert((psx_irq_read_stat(&machine.irq) & 0x0070u) == 0x0070u);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));

    /* Acknowledge each timer independently; IP2 stays asserted until
     * the final pending timer source has been cleared. */
    for (unsigned source = 4u; source <= 6u; ++source) {
        psx_irq_write_stat(&machine.irq,
            (uint16_t)~(1u << source));
        psx_machine_refresh_interrupts(&machine);
        if (source != 6u) {
            assert((psx_irq_read_stat(&machine.irq) & 0x0070u) != 0u);
            assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
            assert(psx_machine_interrupt_eligible(&machine));
        }
    }
    assert((psx_irq_read_stat(&machine.irq) & 0x0070u) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    /* GPU IRQ1 must coexist with all three timer IRQ sources while
     * the mixed DOT/HBlank/SYSTEM/8 clocks advance one scanline. Device
     * acknowledgement and I_STAT acknowledgement are independent. */
    psx_machine_reset(&machine);
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000001u));
    psx_irq_write_mask(&machine.irq, (uint16_t)(0x0070u | (1u << 1u)));
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    for (unsigned timer = 0u; timer < 3u; ++timer) {
        const uint32_t base = PSX_TIMER_BASE_ADDR +
                              timer * PSX_TIMER_STRIDE;
        assert(psx_timers_write16(&machine.timers, base + 8u, 1u));
        assert(psx_timers_write16(&machine.timers, base + 4u,
            (uint16_t)((1u << PSX_TIMER_MODE_CLOCK_SHIFT) |
                       PSX_TIMER_MODE_IRQ_TARGET |
                       PSX_TIMER_MODE_IRQ_REPEAT)));
    }
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert(psx_machine_advance_system_cycles(&machine, 2153u));
    assert((psx_irq_read_stat(&machine.irq) & 0x0072u) == 0x0072u);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    assert(psx_gpu_gp1_write(&machine.gpu, 0x02000000u));
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 1u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 1u)) == 0u);
    assert((psx_irq_read_stat(&machine.irq) & 0x0070u) == 0x0070u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    for (unsigned source = 4u; source <= 6u; ++source) {
        psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << source));
        psx_machine_refresh_interrupts(&machine);
        if (source != 6u) {
            assert((psx_irq_read_stat(&machine.irq) & 0x0070u) != 0u);
            assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
        }
    }
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* A pending GPU IRQ1 plus Timer0/1/2 IRQs raised by the branch
     * instruction's elapsed cycles must all wait until its delay slot
     * retires. This checks four independent sources at one CPU boundary. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, 0x0072u);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    for (unsigned timer = 0u; timer < 3u; ++timer) {
        const uint32_t base = PSX_TIMER_BASE_ADDR +
                              timer * PSX_TIMER_STRIDE;
        assert(psx_timers_write16(&machine.timers, base + 8u, 1u));
        assert(psx_timers_write16(&machine.timers, base + 4u,
            (uint16_t)(PSX_TIMER_MODE_IRQ_TARGET |
                       PSX_TIMER_MODE_IRQ_REPEAT)));
    }
    assert(psx_bus_write32(&machine.bus, 0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x24090007u)); /* delay: t1 = 7 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x240a0009u)); /* target: t2 = 9 */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);

    assert(psx_machine_step(&machine)); /* Branch retires, timers advance. */
    assert(machine.cpu.delay_slot_pending);
    assert((psx_irq_read_stat(&machine.irq) & 0x0072u) == 0x0072u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(machine.cpu.pc == 4u && machine.cpu.next_pc == 8u);

    assert(psx_machine_step(&machine)); /* All three IRQs defer for delay slot. */
    assert(!machine.cpu.delay_slot_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 7u);
    assert(machine.cpu.pc == 8u && machine.cpu.next_pc == 12u);

    assert(psx_machine_step(&machine)); /* IRQ accepted at next boundary. */
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 8u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) == 0u);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0u);
    assert((psx_irq_read_stat(&machine.irq) & 0x0072u) == 0x0072u);

    /* A synchronous SYSCALL exception must retain simultaneous GPU and
     * Timer0/1/2 requests while COP0 interrupts are disabled. Re-enabling
     * interrupts at the exception vector must then deliver the pending IP2. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, 0x0072u);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    for (unsigned timer = 0u; timer < 3u; ++timer) {
        const uint32_t base = PSX_TIMER_BASE_ADDR +
                              timer * PSX_TIMER_STRIDE;
        assert(psx_timers_write16(&machine.timers, base + 8u, 1u));
        assert(psx_timers_write16(&machine.timers, base + 4u,
                                  PSX_TIMER_MODE_IRQ_TARGET));
        assert(psx_machine_timer_advance_clock(&machine, timer,
            PSX_TIMER_CLOCK_SYSTEM, 1u));
    }
    assert((psx_irq_read_stat(&machine.irq) & 0x0072u) == 0x0072u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x0000000cu)); /* SYSCALL */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u; /* Mask external IRQ while SYSCALL executes. */

    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_SYSCALL <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert((psx_irq_read_stat(&machine.irq) & 0x0072u) == 0x0072u);

    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 0x80000080u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert((psx_irq_read_stat(&machine.irq) & 0x0072u) == 0x0072u);

    /* Acknowledge the GPU device and all four I_STAT sources after the
     * exception path. The shared CPU interrupt must drop only after the
     * final source is cleared, then a fresh GPU and Timer0 event must relatch. */
    assert(psx_gpu_gp1_write(&machine.gpu, 0x02000000u));
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 1u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & 0x0070u) == 0x0070u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    for (unsigned source = 4u; source <= 6u; ++source) {
        psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << source));
        psx_machine_refresh_interrupts(&machine);
        if (source != 6u) {
            assert((psx_irq_read_stat(&machine.irq) & 0x0070u) != 0u);
            assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
        }
    }
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 1u)) != 0u);
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR, 0u));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((psx_irq_read_stat(&machine.irq) & ((1u << 1u) | (1u << 4u))) ==
           ((1u << 1u) | (1u << 4u)));
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* I_MASK gates delivery to COP0 without destroying I_STAT. Enabling
     * either pending source must assert IP2; masking both must drop IP2
     * while both device requests remain latched. */
    psx_machine_reset(&machine);
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 4u, PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((psx_irq_read_stat(&machine.irq) & 0x0012u) == 0x0012u);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 1u));
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));

    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & 0x0012u) == 0x0012u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 4u));
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 4u));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* Guest code writes I_MASK and reads it back through the memory bus.
     * The pending sources must survive until COP0 delivery is enabled. */
    psx_machine_reset(&machine);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 4u, PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((psx_irq_read_stat(&machine.irq) & 0x0012u) == 0x0012u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081074u)); /* ORI t0,t0,1074 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x24090012u)); /* ADDIU t1,zero,12h */
    assert(psx_bus_write32(&machine.bus, 12u, 0xac090000u)); /* SW t1,0(t0) */
    assert(psx_bus_write32(&machine.bus, 16u, 0x8d0a0000u)); /* LW t2,0(t0) */
    assert(psx_bus_write32(&machine.bus, 20u, 0x00000000u)); /* load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u; /* Let guest finish before accepting IRQ. */
    for (unsigned step = 0u; step < 6u; ++step) {
        assert(psx_machine_step(&machine));
    }
    assert(psx_irq_read_mask(&machine.irq) == 0x0012u);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x0012u);
    assert((psx_irq_read_stat(&machine.irq) & 0x0012u) == 0x0012u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 24u);
    assert((psx_irq_read_stat(&machine.irq) & 0x0012u) == 0x0012u);

    /* Guest SW to I_STAT clears only Timer0 (write-zero-to-clear); GPU IRQ1
     * must remain pending and visible to a guest LW of the same register. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, 0x0012u);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 4u, PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((psx_irq_read_stat(&machine.irq) & 0x0012u) == 0x0012u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081070u)); /* ORI t0,t0,1070 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x2409ffefu)); /* ADDIU t1,zero,ffef */
    assert(psx_bus_write32(&machine.bus, 12u, 0xad090000u)); /* SW t1,0(t0) */
    assert(psx_bus_write32(&machine.bus, 16u, 0x8d0a0000u)); /* LW t2,0(t0) */
    assert(psx_bus_write32(&machine.bus, 20u, 0x00000000u)); /* load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u;
    for (unsigned step = 0u; step < 6u; ++step) {
        assert(psx_machine_step(&machine));
    }
    assert(psx_irq_read_stat(&machine.irq) == (1u << 1u));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == (1u << 1u));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* Device acknowledgement followed by guest W0C clears the final source. */
    assert(psx_gpu_gp1_write(&machine.gpu, 0x02000000u));
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 1u)));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* Byte-lane MMIO must preserve the untouched I_STAT byte. A guest SB
     * clears Timer0 in the low byte, then LBU sees the remaining GPU bit. */
    psx_machine_reset(&machine);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 4u, PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((psx_irq_read_stat(&machine.irq) & 0x0012u) == 0x0012u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081070u)); /* ORI t0,t0,1070 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x240900efu)); /* ADDIU t1,zero,ef */
    assert(psx_bus_write32(&machine.bus, 12u, 0xa1090000u)); /* SB t1,0(t0) */
    assert(psx_bus_write32(&machine.bus, 16u, 0x910a0000u)); /* LBU t2,0(t0) */
    assert(psx_bus_write32(&machine.bus, 20u, 0x00000000u)); /* load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u;
    for (unsigned step = 0u; step < 6u; ++step)
        assert(psx_machine_step(&machine));
    assert(psx_irq_read_stat(&machine.irq) == (1u << 1u));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == (1u << 1u));

    /* The upper byte lane must use address bit 0 correctly too. Guest SB
     * clears source 10 while preserving the pending GPU request in byte 0. */
    psx_machine_reset(&machine);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    psx_irq_set_line(&machine.irq, 10u, true);
    assert((psx_irq_read_stat(&machine.irq) & 0x0402u) == 0x0402u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081071u)); /* ORI t0,t0,1071 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x240900fbu)); /* ADDIU t1,zero,fb */
    assert(psx_bus_write32(&machine.bus, 12u, 0xa1090000u)); /* SB t1,0(t0) */
    assert(psx_bus_write32(&machine.bus, 16u, 0x910a0000u)); /* LBU t2,0(t0) */
    assert(psx_bus_write32(&machine.bus, 20u, 0x00000000u)); /* load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u;
    for (unsigned step = 0u; step < 6u; ++step)
        assert(psx_machine_step(&machine));
    assert(psx_irq_read_stat(&machine.irq) == (1u << 1u));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0u);

    /* Guest byte stores to both I_MASK lanes must merge rather than
     * overwrite the other byte. LBU verifies each lane through the CPU bus. */
    psx_machine_reset(&machine);
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081074u)); /* ORI t0,t0,1074 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x24090012u)); /* ADDIU t1,zero,12 */
    assert(psx_bus_write32(&machine.bus, 12u, 0xa1090000u)); /* SB low byte */
    assert(psx_bus_write32(&machine.bus, 16u, 0x910a0000u)); /* LBU t2,low */
    assert(psx_bus_write32(&machine.bus, 20u, 0x00000000u)); /* load delay */
    assert(psx_bus_write32(&machine.bus, 24u, 0x240b0004u)); /* ADDIU t3,zero,4 */
    assert(psx_bus_write32(&machine.bus, 28u, 0xa10b0001u)); /* SB high byte */
    assert(psx_bus_write32(&machine.bus, 32u, 0x910c0001u)); /* LBU t4,high */
    assert(psx_bus_write32(&machine.bus, 36u, 0x00000000u)); /* load delay */
    assert(psx_bus_write32(&machine.bus, 40u, 0x24090000u)); /* clear low value */
    assert(psx_bus_write32(&machine.bus, 44u, 0xa1090000u)); /* SB low byte */
    assert(psx_bus_write32(&machine.bus, 48u, 0x910d0001u)); /* LBU t5,high */
    assert(psx_bus_write32(&machine.bus, 52u, 0x00000000u)); /* load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u;
    for (unsigned step = 0u; step < 14u; ++step)
        assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x12u);
    assert(psx_cpu_read_gpr(&machine.cpu, 12u) == 0x04u);
    assert(psx_cpu_read_gpr(&machine.cpu, 13u) == 0x04u);
    assert(psx_irq_read_mask(&machine.irq) == 0x0400u);

    /* Guest halfword access to I_STAT: SH performs W0C on Timer0 only,
     * then LHU must observe the still-pending GPU IRQ1 request. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, 0x0012u);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 4u, PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081070u)); /* ORI t0,t0,1070 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x2409ffefu)); /* ADDIU t1,zero,ffef */
    assert(psx_bus_write32(&machine.bus, 12u, 0xa5090000u)); /* SH t1,0(t0) */
    assert(psx_bus_write32(&machine.bus, 16u, 0x950a0000u)); /* LHU t2,0(t0) */
    assert(psx_bus_write32(&machine.bus, 20u, 0x00000000u)); /* load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u;
    for (unsigned step = 0u; step < 6u; ++step)
        assert(psx_machine_step(&machine));
    assert(psx_irq_read_stat(&machine.irq) == (1u << 1u));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == (1u << 1u));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* Guest word access to I_STAT follows the same W0C semantics as
     * SH: clear Timer0 while retaining GPU IRQ1, then verify with LW. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, 0x0012u);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 4u, PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081070u)); /* ORI t0,t0,1070 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x2409ffefu)); /* ADDIU t1,zero,ffef */
    assert(psx_bus_write32(&machine.bus, 12u, 0xad090000u)); /* SW t1,0(t0) */
    assert(psx_bus_write32(&machine.bus, 16u, 0x8d0a0000u)); /* LW t2,0(t0) */
    assert(psx_bus_write32(&machine.bus, 20u, 0x00000000u)); /* load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u;
    for (unsigned step = 0u; step < 6u; ++step)
        assert(psx_machine_step(&machine));
    assert(psx_irq_read_stat(&machine.irq) == (1u << 1u));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == (1u << 1u));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* Guest I_MASK writes must immediately gate COP0 IP2 without
     * destroying the independently latched GPU and Timer0 requests. */
    psx_machine_reset(&machine);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 4u, PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(psx_irq_read_stat(&machine.irq) == 0x0012u);
    assert(psx_irq_read_mask(&machine.irq) == 0u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081074u)); /* ORI t0,t0,1074 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x24090012u)); /* ADDIU t1,zero,12 */
    assert(psx_bus_write32(&machine.bus, 12u, 0xad090000u)); /* enable GPU+Timer0 */
    assert(psx_bus_write32(&machine.bus, 16u, 0x8d0a0000u)); /* LW t2,I_MASK */
    assert(psx_bus_write32(&machine.bus, 20u, 0x00000000u)); /* load delay */
    assert(psx_bus_write32(&machine.bus, 24u, 0x24090000u)); /* ADDIU t1,zero,0 */
    assert(psx_bus_write32(&machine.bus, 28u, 0xad090000u)); /* mask off */
    assert(psx_bus_write32(&machine.bus, 32u, 0x8d0b0000u)); /* LW t3,I_MASK */
    assert(psx_bus_write32(&machine.bus, 36u, 0x00000000u)); /* load delay */
    assert(psx_bus_write32(&machine.bus, 40u, 0x24090010u)); /* Timer0 only */
    assert(psx_bus_write32(&machine.bus, 44u, 0xad090000u)); /* enable Timer0 */
    assert(psx_bus_write32(&machine.bus, 48u, 0x8d0c0000u)); /* LW t4,I_MASK */
    assert(psx_bus_write32(&machine.bus, 52u, 0x00000000u)); /* load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u;
    for (unsigned step = 0u; step < 4u; ++step)
        assert(psx_machine_step(&machine));
    assert(psx_irq_read_mask(&machine.irq) == 0x0012u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    for (unsigned step = 4u; step < 8u; ++step)
        assert(psx_machine_step(&machine));
    assert(psx_irq_read_mask(&machine.irq) == 0u);
    assert((psx_irq_read_stat(&machine.irq) & 0x0012u) == 0x0012u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    for (unsigned step = 8u; step < 14u; ++step)
        assert(psx_machine_step(&machine));
    assert(psx_irq_read_mask(&machine.irq) == 0x0010u);
    assert((psx_irq_read_stat(&machine.irq) & 0x0012u) == 0x0012u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x0012u);
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 0u);
    assert(psx_cpu_read_gpr(&machine.cpu, 12u) == 0x0010u);

    /* A guest W0C acknowledgement must update the real CPU interrupt
     * decision: Timer0 is cleared, GPU IRQ1 remains, so COP0 accepts it. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, 0x0012u);
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 4u, PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(psx_bus_write32(&machine.bus, 0u, 0x3c081f80u)); /* LUI t0,1f80 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x35081070u)); /* ORI t0,t0,1070 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x2409ffefu)); /* keep GPU, clear T0 */
    assert(psx_bus_write32(&machine.bus, 12u, 0xad090000u)); /* SW t1,I_STAT */
    assert(psx_bus_write32(&machine.bus, 16u, 0x8d0a0000u)); /* LW t2,I_STAT */
    assert(psx_bus_write32(&machine.bus, 20u, 0x00000000u)); /* load delay */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    machine.cop0_status = 0u;
    for (unsigned step = 0u; step < 6u; ++step)
        assert(psx_machine_step(&machine));
    assert(psx_irq_read_stat(&machine.irq) == (1u << 1u));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == (1u << 1u));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 24u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_gpu_gp1_write(&machine.gpu, 0x02000000u));
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 1u)));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    /* A fresh GPU command after acknowledgement must create a new edge
     * and relatch IRQ1; the prior exception must not erase the new request. */
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == (1u << 1u));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(!psx_machine_interrupt_eligible(&machine)); /* EXL still set */
    assert(psx_gpu_gp1_write(&machine.gpu, 0x02000000u));
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 1u)));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* RFE restores the pre-exception interrupt-enable stack. If the
     * device request is still pending, the next boundary must re-enter. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 1u));
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_bus_write32(&machine.bus, 0x80u, 0x42000010u)); /* RFE */
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_machine_interrupt_eligible(&machine));
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == PSX_CPU_RESET_PC);
    assert((machine.cop0_status & PSX_COP0_STATUS_IEC) == 0u);
    assert(psx_machine_step(&machine)); /* execute RFE */
    assert((machine.cop0_status & PSX_COP0_STATUS_IEC) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_step(&machine)); /* still-pending IRQ re-enters */
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 0x80000084u);
    assert((machine.cop0_status & PSX_COP0_STATUS_IEC) == 0u);
    assert(psx_gpu_gp1_write(&machine.gpu, 0x02000000u));
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 1u)));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* RFE must re-deliver a combined GPU+Timer0 request. Clearing one
     * source after re-entry must leave the other source asserted on IP2. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, 0x0012u);
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_bus_write32(&machine.bus, 0x80u, 0x42000010u)); /* RFE */
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_timers_write16(&machine.timers,
        PSX_TIMER_BASE_ADDR + 4u, PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0x0012u);
    assert(psx_machine_step(&machine)); /* first interrupt */
    assert(machine.cpu.pc == 0x80000080u);
    assert(psx_machine_step(&machine)); /* RFE */
    assert((machine.cop0_status & PSX_COP0_STATUS_IEC) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_step(&machine)); /* pending sources re-enter */
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 0x80000084u);
    assert(psx_gpu_gp1_write(&machine.gpu, 0x02000000u));
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 1u)));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == (1u << 4u));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 4u)));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* All three timer IRQs plus GPU IRQ1 must survive exception entry
     * and RFE. Each source is acknowledged independently after re-entry. */
    psx_machine_reset(&machine);
    psx_irq_write_mask(&machine.irq, 0x0072u);
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    assert(psx_bus_write32(&machine.bus, 0x80u, 0x42000010u)); /* RFE */
    for (unsigned timer = 0u; timer < PSX_TIMER_COUNT; ++timer) {
        const uint32_t base = PSX_TIMER_BASE_ADDR +
                              timer * PSX_TIMER_STRIDE;
        assert(psx_timers_write16(&machine.timers, base + 8u, 1u));
        assert(psx_timers_write16(&machine.timers, base + 4u,
                                  PSX_TIMER_MODE_IRQ_TARGET));
        assert(psx_machine_timer_advance_clock(&machine, timer,
            PSX_TIMER_CLOCK_SYSTEM, 1u));
    }
    assert(psx_gpu_gp0_write(&machine.gpu, 0x1f000000u));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0x0072u);
    assert(psx_machine_step(&machine)); /* first interrupt */
    assert(machine.cpu.pc == 0x80000080u);
    assert(psx_machine_step(&machine)); /* RFE */
    assert((machine.cop0_status & PSX_COP0_STATUS_IEC) != 0u);
    assert(psx_machine_step(&machine)); /* all four sources re-enter */
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 0x80000084u);
    assert(psx_gpu_gp1_write(&machine.gpu, 0x02000000u));
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                           (uint16_t)~(1u << 1u)));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_irq_read_stat(&machine.irq) == 0x0070u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    for (unsigned timer = 0u; timer < PSX_TIMER_COUNT; ++timer) {
        const uint16_t bit = (uint16_t)(1u << (timer + 4u));
        assert(psx_bus_write16(&machine.bus, PSX_IRQ_STAT_ADDR,
                               (uint16_t)~bit));
        psx_machine_refresh_interrupts(&machine);
        assert(psx_irq_read_stat(&machine.irq) ==
               (uint16_t)(0x0070u & (uint16_t)~((1u << (timer + 4u)) |
                   ((1u << timer) - 1u << 4u))));
        if (timer + 1u < PSX_TIMER_COUNT)
            assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    }
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    free(vram);
    free(bios);
    free(ram);
    puts("DreamPSX machine video IRQ tests passed");
    return 0;
}
