#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

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

    /* HDS2 selects 368 pixels; exercise all remaining display flags too. */
    assert(psx_gpu_gp1_write(&gpu, 0x080000fdu));
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.width == 368u && info.height == 480u);
    assert(info.pal && info.color_24bit);
    assert(info.interlaced && info.reverse);

    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 16u)) != 0u); /* HDS2 */
    assert((status & (1u << 17u)) != 0u); /* HRes bit 0 */
    assert((status & (1u << 18u)) == 0u); /* HRes bit 1 */
    assert((status & (1u << 19u)) != 0u); /* VRes */
    assert((status & (1u << 20u)) != 0u); /* PAL */
    assert((status & (1u << 21u)) != 0u); /* 24-bit */
    assert((status & (1u << 22u)) != 0u); /* Interlace */
    assert((status & (1u << 14u)) != 0u); /* Reverse flag */

    /* Without HDS2, the two horizontal resolution bits select 512. */
    assert(psx_gpu_gp1_write(&gpu, 0x08000002u));
    assert(psx_gpu_get_display_mode_info(&gpu, &info));
    assert(info.width == 512u && info.height == 240u);
    assert(!info.pal && !info.color_24bit);
    assert(!info.interlaced && !info.reverse);
    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & ((0x7fu << 16u) | (1u << 14u))) == (2u << 17u));

    assert(!psx_gpu_get_display_mode_info(NULL, &info));
    assert(!psx_gpu_get_display_mode_info(&gpu, NULL));
    puts("GPU display mode decoding tests passed");
    return 0;
}
