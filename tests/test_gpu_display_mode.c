#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

static void set_range(PsxGpu *gpu, unsigned x1, unsigned x2,
                      unsigned y1, unsigned y2)
{
    assert(psx_gpu_gp1_write(gpu, 0x06000000u | x1 | (x2 << 12u)));
    assert(psx_gpu_gp1_write(gpu, 0x07000000u | y1 | (y2 << 10u)));
}

int main(void)
{
    PsxGpu gpu;
    PsxGpuDisplayModeInfo info;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint32_t status = 0u;

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.width == 256u && info.height == 240u);
    assert(!info.pal && !info.color_24bit);
    assert(!info.interlaced && !info.reverse);
    assert(!info.range_valid && info.range_width == 0u &&
           info.range_height == 0u);

    /* HDS2 selects 368 pixels; exercise all remaining display flags too. */
    assert(psx_gpu_gp1_write(&gpu, 0x080000fdu));
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.width == 368u && info.height == 480u);
    assert(info.pal && info.color_24bit);
    assert(info.interlaced && info.reverse);
    assert(info.field_valid && !info.field_odd);

    /* 480i field parity is supplied by the future video timing core. */
    assert(!psx_gpu_set_display_field(&gpu, false));
    assert(!psx_gpu_set_display_field(&gpu, true));
    assert(psx_gpu_gp1_write(&gpu, 0x08000024u)); /* 240i: not 480i */
    assert(!psx_gpu_set_display_field(&gpu, true));
    assert(psx_gpu_gp1_write(&gpu, 0x0800003cu)); /* 480i PAL 24-bit */
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.field_valid && !info.field_odd);
    assert(psx_gpu_set_display_field(&gpu, true));
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.field_valid && info.field_odd);
    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 31u)) != 0u);
    assert(psx_gpu_set_display_field(&gpu, false));
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(!info.field_odd);
    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 31u)) == 0u);

    /* Return to the original mode for the remaining mode/status checks. */
    assert(psx_gpu_gp1_write(&gpu, 0x080000fdu));
    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 16u)) != 0u);
    assert((status & (1u << 17u)) != 0u);
    assert((status & (1u << 18u)) == 0u);
    assert((status & (1u << 19u)) != 0u);
    assert((status & (1u << 20u)) != 0u);
    assert((status & (1u << 21u)) != 0u);
    assert((status & (1u << 22u)) != 0u);
    assert((status & (1u << 14u)) != 0u);

    /* The display-range registers determine the actual output rectangle.
     * Each clock span below exercises one documented dot-clock divisor.
     */
    assert(psx_gpu_gp1_write(&gpu, 0x08000000u)); /* 256, /10 */
    set_range(&gpu, 100u, 2640u, 16u, 256u);
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.range_valid && info.range_width == 256u &&
           info.range_height == 240u);

    assert(psx_gpu_gp1_write(&gpu, 0x08000001u)); /* 320, /8 */
    set_range(&gpu, 100u, 2644u, 20u, 260u);
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.range_width == 320u && info.range_height == 240u);

    assert(psx_gpu_gp1_write(&gpu, 0x08000002u)); /* 512, /5 */
    set_range(&gpu, 100u, 2650u, 30u, 270u);
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.range_width == 512u && info.range_height == 240u);

    assert(psx_gpu_gp1_write(&gpu, 0x08000003u)); /* 640, /4 */
    set_range(&gpu, 100u, 2652u, 40u, 280u);
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.range_width == 640u && info.range_height == 240u);

    assert(psx_gpu_gp1_write(&gpu, 0x08000040u)); /* 368, /7 */
    set_range(&gpu, 100u, 2662u, 50u, 530u);
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.range_width == 368u && info.range_height == 480u);

    /* Overscan is preserved; reversed or empty ranges are reported invalid. */
    set_range(&gpu, 100u, 2740u, 10u, 270u);
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.range_width == 376u && info.range_height == 260u);
    set_range(&gpu, 200u, 100u, 10u, 270u);
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(!info.range_valid && info.range_width == 0u &&
           info.range_height == 0u);
    set_range(&gpu, 100u, 100u, 10u, 270u);
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(!info.range_valid);

    assert(!psx_gpu_get_display_mode_info(NULL, &info));
    assert(!psx_gpu_get_display_mode_info(&gpu, NULL));
    puts("GPU display mode and range tests passed");
    return 0;
}
