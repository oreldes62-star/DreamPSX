#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

int main(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint16_t output[8] = {0u, 0u, 0u, 0xaaaau,
                          0u, 0u, 0u, 0xaaaau};

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* Disabled output must not produce a frame. */
    assert(!psx_gpu_scanout_rgb565(&gpu, 3u, 2u, output, 4u));

    vram[511u * PSX_GPU_VRAM_WIDTH + 1022u] = 0x001fu;
    vram[511u * PSX_GPU_VRAM_WIDTH + 1023u] = 0x03e0u;
    vram[511u * PSX_GPU_VRAM_WIDTH] = 0x7c00u;
    vram[1022u] = 0x7fffu;
    vram[1023u] = 0u;
    vram[0u] = 0x7fe0u;

    /* Set scanout origin to the last two columns and final VRAM row. */
    assert(psx_gpu_gp1_write(&gpu, 0x05000000u | 1022u | (511u << 10u)));
    assert(psx_gpu_gp1_write(&gpu, 0x03000000u));
    assert(psx_gpu_scanout_rgb565(&gpu, 3u, 2u, output, 4u));

    assert(output[0] == 0xf800u);
    assert(output[1] == 0x07e0u);
    assert(output[2] == 0x001fu);
    assert(output[3] == 0xaaaau);
    assert(output[4] == 0xffffu);
    assert(output[5] == 0u);
    assert(output[6] == 0x07ffu);
    assert(output[7] == 0xaaaau);

    /* 24-bit pixels are packed RGB bytes within each VRAM scanline.
     * The first pixel starts at byte 2044; later pixels cross the row edge.
     */
    assert(psx_gpu_gp1_write(&gpu, 0x08000010u));
    vram[511u * PSX_GPU_VRAM_WIDTH + 1022u] = 0x80ffu;
    vram[511u * PSX_GPU_VRAM_WIDTH + 1023u] = 0x40u;
    vram[511u * PSX_GPU_VRAM_WIDTH] = 0xff20u;
    vram[511u * PSX_GPU_VRAM_WIDTH + 1u] = 0x0000u;
    vram[511u * PSX_GPU_VRAM_WIDTH + 2u] = 0x00ffu;
    /* Next scanline is independently read from VRAM row zero. */
    vram[1022u] = 0x00ffu;
    vram[1023u] = 0x00ffu;
    vram[0u] = 0x0000u;
    vram[1u] = 0x0000u;
    vram[2u] = 0x00ffu;
    assert(psx_gpu_scanout_rgb565(&gpu, 3u, 2u, output, 4u));
    assert(output[0] == 0xfc08u); /* (255,128,64) */
    assert(output[1] == 0x011fu); /* (0,32,255) */
    assert(output[2] == 0x001fu); /* (0,0,255) */
    assert(output[3] == 0xaaaau);
    assert(output[4] == 0xf81fu); /* (255,0,255) */
    assert(output[5] == 0u);    /* (0,0,0) */
    assert(output[6] == 0x001fu); /* (0,0,255) */
    assert(output[7] == 0xaaaau);

    /* Reject invalid dimensions and destination stride. */
    assert(psx_gpu_gp1_write(&gpu, 0x08000000u));
    assert(!psx_gpu_scanout_rgb565(&gpu, 0u, 2u, output, 4u));
    assert(!psx_gpu_scanout_rgb565(&gpu, 3u, 2u, output, 2u));
    assert(!psx_gpu_scanout_rgb565(&gpu, 3u, 2u, NULL, 4u));
    assert(!psx_gpu_scanout_rgb565(&gpu, PSX_GPU_VRAM_WIDTH + 1u,
                                   1u, output, PSX_GPU_VRAM_WIDTH + 1u));

    puts("GPU display scanout tests passed");
    return 0;
}
