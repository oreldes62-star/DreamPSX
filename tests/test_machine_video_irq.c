#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dreampsx/cop0.h"
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
    psx_irq_write_mask(&machine.irq, 1u << 0u);
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;

    assert(psx_machine_video_advance_scanline(&machine, &video));
    assert(video.vblank && video.vblank_started);
    assert((psx_irq_read_stat(&machine.irq) & 1u) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));

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

    free(vram);
    free(bios);
    free(ram);
    puts("DreamPSX machine video IRQ tests passed");
    return 0;
}
