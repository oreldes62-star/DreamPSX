#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

int main(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint16_t output[8] = {0xffffu, 0xffffu, 0xffffu, 0xaaaau,
                          0xffffu, 0xffffu, 0xffffu, 0xaaaau};

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* RGB555 primary colors and white convert exactly to RGB565. */
    vram[10u * PSX_GPU_VRAM_WIDTH + 20u] = 0x001fu;
    vram[10u * PSX_GPU_VRAM_WIDTH + 21u] = 0x03e0u;
    vram[10u * PSX_GPU_VRAM_WIDTH + 22u] = 0x7c00u;
    vram[11u * PSX_GPU_VRAM_WIDTH + 20u] = 0x7fffu;
    assert(psx_gpu_copy_rgb565(&gpu, 20u, 10u, 3u, 2u, output, 4u));
    assert(output[0] == 0x001fu);
    assert(output[1] == 0x07e0u);
    assert(output[2] == 0xf800u);
    assert(output[3] == 0xaaaau); /* Row padding is untouched. */
    assert(output[4] == 0xffffu);
    assert(output[7] == 0xaaaau);

    /* Reject invalid destinations, strides and VRAM rectangles. */
    assert(!psx_gpu_copy_rgb565(&gpu, 0u, 0u, 3u, 1u, output, 2u));
    assert(!psx_gpu_copy_rgb565(&gpu, PSX_GPU_VRAM_WIDTH - 1u, 0u,
                                2u, 1u, output, 2u));
    assert(!psx_gpu_copy_rgb565(&gpu, 0u, PSX_GPU_VRAM_HEIGHT - 1u,
                                1u, 2u, output, 1u));
    assert(!psx_gpu_copy_rgb565(&gpu, 0u, 0u, 1u, 1u, NULL, 1u));
    assert(!psx_gpu_copy_rgb565(&gpu, 0u, 0u, 0u, 1u, output, 1u));

    puts("GPU RGB565 framebuffer tests passed");
    return 0;
}
