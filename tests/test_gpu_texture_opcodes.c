#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

static void write_flat_textured(PsxGpu *gpu, uint32_t opcode,
                                uint32_t color, unsigned x, unsigned y)
{
    assert(psx_gpu_gp0_write(gpu, opcode | color));
    assert(psx_gpu_gp0_write(gpu, x | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(gpu, (x + 4u) | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(gpu, x | ((y + 4u) << 16u)));
    assert(psx_gpu_gp0_write(gpu, 1u | (1u << 8u)));
}

static void write_gouraud_textured(PsxGpu *gpu, uint32_t opcode,
                                   unsigned x, unsigned y)
{
    assert(psx_gpu_gp0_write(gpu, opcode));
    assert(psx_gpu_gp0_write(gpu, x | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(gpu, 0x00000040u));
    assert(psx_gpu_gp0_write(gpu, (x + 4u) | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(gpu, 0x00000080u));
    assert(psx_gpu_gp0_write(gpu, x | ((y + 4u) << 16u)));
    assert(psx_gpu_gp0_write(gpu, 1u | (1u << 8u)));
}

static uint16_t read_pixel(PsxGpu *gpu, unsigned x, unsigned y)
{
    uint16_t pixel = 0u;
    assert(psx_gpu_vram_read(gpu, x, y, &pixel));
    return pixel;
}

int main(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* 26h: modulated semi-transparent texture; modulation halves red. */
    vram[PSX_GPU_VRAM_WIDTH + 1u] = 0x801fu;
    vram[101u * PSX_GPU_VRAM_WIDTH + 101u] = 0x03e0u;
    write_flat_textured(&gpu, 0x26000000u, 0x00000040u, 100u, 100u);
    assert(read_pixel(&gpu, 101u, 101u) == 0x01e7u);

    /* 27h: raw semi-transparent texture blends the unmodulated texel. */
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0x801fu;
    vram[121u * PSX_GPU_VRAM_WIDTH + 121u] = 0x03e0u;
    write_flat_textured(&gpu, 0x27000000u, 0u, 120u, 120u);
    assert(read_pixel(&gpu, 121u, 121u) == 0x01efu);

    /* 35h: Gouraud raw texture ignores vertex colors and preserves STP. */
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0x801fu;
    vram[141u * PSX_GPU_VRAM_WIDTH + 141u] = 0x03e0u;
    write_gouraud_textured(&gpu, 0x35000000u, 140u, 140u);
    assert(read_pixel(&gpu, 141u, 141u) == 0x801fu);

    /* 37h: Gouraud raw semi-transparent texture blends the raw texel. */
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0x801fu;
    vram[161u * PSX_GPU_VRAM_WIDTH + 161u] = 0x03e0u;
    write_gouraud_textured(&gpu, 0x37000000u, 160u, 160u);
    assert(read_pixel(&gpu, 161u, 161u) == 0x01efu);

    puts("GPU textured opcode matrix tests passed");
    return 0;
}
