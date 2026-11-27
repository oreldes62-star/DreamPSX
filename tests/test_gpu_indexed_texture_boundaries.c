#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

static void draw_indexed_triangle(PsxGpu *gpu, unsigned x, unsigned y,
                                  unsigned u, unsigned v,
                                  unsigned clut, unsigned tpage)
{
    const uint32_t uv = (u & 0xffu) | ((v & 0xffu) << 8u);
    assert(psx_gpu_gp0_write(gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(gpu, x | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, uv | (clut << 16u)));
    assert(psx_gpu_gp0_write(gpu, (x + 4u) | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, uv | (tpage << 16u)));
    assert(psx_gpu_gp0_write(gpu, x | ((y + 4u) << 16u)));
    assert(psx_gpu_gp0_write(gpu, uv));
}

static uint16_t read_pixel(PsxGpu *gpu, unsigned x, unsigned y)
{
    uint16_t value = 0u;
    assert(psx_gpu_vram_read(gpu, x, y, &value));
    return value;
}

static void test_4bit_nibble_and_clut_lookup(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* U=1 selects the high nibble (index A) of the first packed word. */
    vram[1u * PSX_GPU_VRAM_WIDTH] = 0x00a0u;
    vram[20u * PSX_GPU_VRAM_WIDTH + 74u] = 0x4210u;
    draw_indexed_triangle(&gpu, 100u, 100u, 1u, 1u,
                          4u | (20u << 6u), 0u);
    assert(read_pixel(&gpu, 101u, 101u) == 0x4210u);
}

static void test_8bit_high_byte_and_clut_wrap(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* Page 15 starts at word 960; U=1 selects the high byte (index 0x5A). */
    vram[1u * PSX_GPU_VRAM_WIDTH + 960u] = 0x5a00u;
    /* CLUT X=1008 plus index 90 wraps to X=74. */
    vram[20u * PSX_GPU_VRAM_WIDTH + 74u] = 0x6318u;
    draw_indexed_triangle(&gpu, 120u, 100u, 1u, 1u,
                          63u | (20u << 6u), 0x80u | 0x0fu);
    assert(read_pixel(&gpu, 121u, 101u) == 0x6318u);
}

static void test_zero_index_is_transparent(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    vram[1u * PSX_GPU_VRAM_WIDTH] = 0u;
    vram[30u * PSX_GPU_VRAM_WIDTH + 32u] = 0x7fffu;
    vram[151u * PSX_GPU_VRAM_WIDTH + 151u] = 0x03e0u;
    draw_indexed_triangle(&gpu, 150u, 150u, 1u, 1u,
                          2u | (30u << 6u), 0u);
    assert(read_pixel(&gpu, 151u, 151u) == 0x03e0u);
}

int main(void)
{
    test_4bit_nibble_and_clut_lookup();
    test_8bit_high_byte_and_clut_wrap();
    test_zero_index_is_transparent();
    puts("GPU indexed texture boundary tests passed");
    return 0;
}
