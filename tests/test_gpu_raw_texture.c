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

    /* A raw textured primitive must preserve the sampled STP texel verbatim. */
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0x801fu;
    vram[101u * PSX_GPU_VRAM_WIDTH + 101u] = 0x03e0u;

    /* A non-default global blend mode must not affect opaque raw opcode 25h. */
    assert(psx_gpu_gp0_write(&gpu, 0xe1000040u));
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 100u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 104u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u) | (0x0100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 100u | (104u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));

    assert(psx_gpu_vram_read(&gpu, 101u, 101u, &pixel));
    assert(pixel == 0x801fu);

    puts("GPU raw textured triangle tests passed");
    return 0;
}
