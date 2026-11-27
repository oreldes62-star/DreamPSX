#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

static void draw_raw_semitransparent(PsxGpu *gpu, unsigned x, unsigned y,
                                     unsigned blend_mode)
{
    const unsigned tpage = 0x100u | ((blend_mode & 3u) << 5u);
    assert(psx_gpu_gp0_write(gpu, 0x27000000u));
    assert(psx_gpu_gp0_write(gpu, x | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(gpu, (x + 4u) | (y << 16u)));
    assert(psx_gpu_gp0_write(gpu, 1u | (1u << 8u) | (tpage << 16u)));
    assert(psx_gpu_gp0_write(gpu, x | ((y + 4u) << 16u)));
    assert(psx_gpu_gp0_write(gpu, 1u | (1u << 8u)));
}

static uint16_t pixel(PsxGpu *gpu, unsigned x, unsigned y)
{
    uint16_t value = 0u;
    assert(psx_gpu_vram_read(gpu, x, y, &value));
    return value;
}

static void test_blend_modes(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    static const uint16_t expected[4] = {
        0x01efu, /* average */
        0x03ffu, /* add, saturated */
        0x03e0u, /* subtract */
        0x03e7u  /* add quarter */
    };

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    for (unsigned mode = 0u; mode < 4u; ++mode) {
        const unsigned x = 100u + mode * 10u;
        const unsigned y = 100u;
        vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0x801fu;
        vram[(y + 1u) * PSX_GPU_VRAM_WIDTH + x + 1u] = 0x03e0u;
        draw_raw_semitransparent(&gpu, x, y, mode);
        assert(pixel(&gpu, x + 1u, y + 1u) == expected[mode]);
    }

    /* An STP-clear texel must replace the destination, not blend with it. */
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0x001fu;
    vram[151u * PSX_GPU_VRAM_WIDTH + 151u] = 0x03e0u;
    draw_raw_semitransparent(&gpu, 150u, 150u, 0u);
    assert(pixel(&gpu, 151u, 151u) == 0x001fu);
}

static void test_uv_interpolation(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    /* Distinct texels make the interpolated (U,V) observable. */
    vram[2u * PSX_GPU_VRAM_WIDTH + 2u] = 0x4210u;
    vram[2u * PSX_GPU_VRAM_WIDTH + 3u] = 0x1234u;
    vram[3u * PSX_GPU_VRAM_WIDTH + 2u] = 0x2345u;
    vram[200u * PSX_GPU_VRAM_WIDTH + 200u] = 0x7fffu;

    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 200u | (200u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 204u | (200u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 5u | (1u << 8u) | (0x100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 200u | (204u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (5u << 8u)));

    /* At pixel center (201.5,201.5), barycentric UV rounds down to (2,2). */
    assert(pixel(&gpu, 201u, 201u) == 0x4210u);
}

int main(void)
{
    test_blend_modes();
    test_uv_interpolation();
    puts("GPU texture rasterization tests passed");
    return 0;
}
