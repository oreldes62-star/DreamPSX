#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

static void draw_textured_triangle(PsxGpu *gpu, unsigned x, unsigned y,
                                   uint32_t uv0, uint32_t uv1,
                                   uint32_t uv2)
{
    assert(psx_gpu_gp0_write(gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(gpu, x | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, uv0));
    assert(psx_gpu_gp0_write(gpu, (x + 4u) | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, uv1));
    assert(psx_gpu_gp0_write(gpu, x | ((y + 4u) << 16u)));
    assert(psx_gpu_gp0_write(gpu, uv2));
}

int main(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint16_t pixel = 0u;

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* Direct 15-bit sampling wraps U across the right VRAM edge. */
    vram[511u * PSX_GPU_VRAM_WIDTH + 191u] = 0x4210u;
    draw_textured_triangle(&gpu, 100u, 100u,
                           255u | (255u << 8u),
                           255u | (255u << 8u) | (0x011fu << 16u),
                           255u | (255u << 8u));
    assert(psx_gpu_vram_read(&gpu, 101u, 101u, &pixel));
    assert(pixel == 0x4210u);

    /* 8-bit packed texel fetch wraps X; the CLUT lookup wraps independently. */
    vram[10u * PSX_GPU_VRAM_WIDTH + 63u] = 0x5a00u;
    vram[30u * PSX_GPU_VRAM_WIDTH + 74u] = 0x6318u;
    draw_textured_triangle(&gpu, 120u, 100u,
                           255u | (10u << 8u) | (63u << 16u),
                           255u | (10u << 8u) | (0x008fu << 16u),
                           255u | (10u << 8u) | (63u << 16u));
    assert(psx_gpu_vram_read(&gpu, 121u, 101u, &pixel));
    assert(pixel == 0x6318u);

    /* A0 image upload wraps both axes at the physical VRAM boundary. */
    assert(psx_gpu_gp0_write(&gpu, 0xa0000000u));
    assert(psx_gpu_gp0_write(&gpu, 1023u | (511u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 2u | (2u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x22221111u));
    assert(psx_gpu_gp0_write(&gpu, 0x44443333u));

    assert(psx_gpu_vram_read(&gpu, 1023u, 511u, &pixel));
    assert(pixel == 0x1111u);
    assert(psx_gpu_vram_read(&gpu, 0u, 511u, &pixel));
    assert(pixel == 0x2222u);
    assert(psx_gpu_vram_read(&gpu, 1023u, 0u, &pixel));
    assert(pixel == 0x3333u);
    assert(psx_gpu_vram_read(&gpu, 0u, 0u, &pixel));
    assert(pixel == 0x4444u);

    puts("GPU texture and VRAM wrap tests passed");
    return 0;
}
