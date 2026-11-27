#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

static void draw_raw_semitransparent_triangle(PsxGpu *gpu,
                                               unsigned x0, unsigned y0,
                                               unsigned x1, unsigned y1,
                                               unsigned x2, unsigned y2)
{
    const unsigned uv = 1u | (1u << 8u);
    const unsigned uv_tpage = uv | (0x100u << 16u);
    assert(psx_gpu_gp0_write(gpu, 0x27000000u));
    assert(psx_gpu_gp0_write(gpu, x0 | (y0 << 16u)));
    assert(psx_gpu_gp0_write(gpu, uv));
    assert(psx_gpu_gp0_write(gpu, x1 | (y1 << 16u)));
    assert(psx_gpu_gp0_write(gpu, uv_tpage));
    assert(psx_gpu_gp0_write(gpu, x2 | (y2 << 16u)));
    assert(psx_gpu_gp0_write(gpu, uv));
}

static uint16_t read_pixel(PsxGpu *gpu, unsigned x, unsigned y)
{
    uint16_t value = 0u;
    assert(psx_gpu_vram_read(gpu, x, y, &value));
    return value;
}

static void test_shared_diagonal_is_covered_once(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    const unsigned left = 40u;
    const unsigned top = 40u;
    const uint16_t background = 0x03e0u;
    const uint16_t stp_red = 0x801fu;
    const uint16_t blended_once = 0x01efu;

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* Raw 15-bit texture page: the STP texel is at UV=(1,1). */
    vram[PSX_GPU_VRAM_WIDTH + 1u] = stp_red;
    for (unsigned y = top; y < top + 4u; ++y) {
        for (unsigned x = left; x < left + 4u; ++x)
            vram[y * PSX_GPU_VRAM_WIDTH + x] = background;
    }

    /* Two triangles form a 4x4 square and share its diagonal. */
    draw_raw_semitransparent_triangle(&gpu,
        left, top, left + 4u, top, left + 4u, top + 4u);
    draw_raw_semitransparent_triangle(&gpu,
        left, top, left + 4u, top + 4u, left, top + 4u);

    /* Every pixel is covered, and the shared edge is never blended twice. */
    for (unsigned y = top; y < top + 4u; ++y) {
        for (unsigned x = left; x < left + 4u; ++x)
            assert(read_pixel(&gpu, x, y) == blended_once);
    }
}

static void test_reversed_winding_covers_same_triangle(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    const unsigned left = 80u;
    const unsigned top = 80u;

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    vram[PSX_GPU_VRAM_WIDTH + 1u] = 0x001fu;

    draw_raw_semitransparent_triangle(&gpu,
        left, top, left + 4u, top, left, top + 4u);
    const uint16_t clockwise = read_pixel(&gpu, left + 1u, top + 1u);

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    vram[PSX_GPU_VRAM_WIDTH + 1u] = 0x001fu;
    draw_raw_semitransparent_triangle(&gpu,
        left, top + 4u, left + 4u, top, left, top);
    assert(read_pixel(&gpu, left + 1u, top + 1u) == clockwise);
}

int main(void)
{
    test_shared_diagonal_is_covered_once();
    test_reversed_winding_covers_same_triangle();
    puts("GPU textured shared-edge tests passed");
    return 0;
}
