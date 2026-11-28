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
    assert(psx_machine_timer_advance_clock(&machine, 1u,
                                            PSX_TIMER_CLOCK_HBLANK, 1u));
    assert(machine.timers.timer[1].counter == 2u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 5u)) == 0u);

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
    assert(psx_machine_advance_system_cycles(&machine, 2147u));
    assert(machine.gpu.display.video_scanline == 0u);
    assert(machine.gpu_scanline_cycle_phase == 2147u);
    assert(psx_machine_advance_system_cycles(&machine, 1u));
    assert(machine.gpu.display.video_scanline == 1u);
    assert(machine.gpu_scanline_cycle_phase == 0u);
    assert(machine.gpu.display.video_vblank);

    /* PAL uses its own line cadence and likewise survives split intervals. */
    psx_gpu_reset(&machine.gpu);
    assert(psx_gpu_gp1_write(&machine.gpu,
        0x07000000u | (256u << 10u) | 16u));
    assert(psx_gpu_gp1_write(&machine.gpu, 0x08000008u));
    machine.gpu_scanline_cycle_phase = 0u;
    assert(psx_machine_advance_system_cycles(&machine, 2156u));
    assert(machine.gpu.display.video_scanline == 0u);
    assert(machine.gpu_scanline_cycle_phase == 2156u);
    assert(psx_machine_advance_system_cycles(&machine, 1u));
    assert(machine.gpu.display.video_scanline == 1u);
    assert(machine.gpu_scanline_cycle_phase == 0u);

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

    free(vram);
    free(bios);
    free(ram);
    puts("DreamPSX machine video IRQ tests passed");
    return 0;
}
