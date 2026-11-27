#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

int main(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint16_t pixel = 0u;

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* A constant UV maps every covered fragment to this direct-color texel. */
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0x7c00u;
    vram[200u * PSX_GPU_VRAM_WIDTH + 200u] = 0x1234u;
    vram[201u * PSX_GPU_VRAM_WIDTH + 200u] = 0x2345u;
    vram[201u * PSX_GPU_VRAM_WIDTH + 203u] = 0x3456u;

    /* Clip the textured primitive to a small inclusive rectangle. */
    assert(psx_gpu_gp0_write(&gpu, 0xe3000000u | 201u | (201u << 10u)));
    assert(psx_gpu_gp0_write(&gpu, 0xe4000000u | 202u | (202u << 10u)));

    /* The draw offset moves the submitted triangle into the clip rectangle. */
    assert(psx_gpu_gp0_write(&gpu, 0xe5000000u | 10u | (20u << 11u)));
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 190u | (180u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 194u | (180u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 190u | (184u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));

    assert(psx_gpu_vram_read(&gpu, 201u, 201u, &pixel));
    assert(pixel == 0x7c00u);

    /* Pixels outside the drawing area remain unchanged despite triangle coverage. */
    assert(psx_gpu_vram_read(&gpu, 200u, 200u, &pixel));
    assert(pixel == 0x1234u);
    assert(psx_gpu_vram_read(&gpu, 200u, 201u, &pixel));
    assert(pixel == 0x2345u);
    assert(psx_gpu_vram_read(&gpu, 203u, 201u, &pixel));
    assert(pixel == 0x3456u);

    puts("GPU textured clipping tests passed");
    return 0;
}
