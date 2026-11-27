#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

static uint16_t pixel(PsxGpu *gpu, unsigned x, unsigned y)
{
    uint16_t value = 0u;
    assert(psx_gpu_vram_read(gpu, x, y, &value));
    return value;
}

int main(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* Variable-size raw 15-bit rectangle increments U and V per pixel. */
    assert(psx_gpu_gp0_write(&gpu, 0xe1000000u | (2u << 7u)));
    vram[6u * PSX_GPU_VRAM_WIDTH + 5u] = 0x001fu;
    vram[6u * PSX_GPU_VRAM_WIDTH + 6u] = 0x03e0u;
    vram[7u * PSX_GPU_VRAM_WIDTH + 5u] = 0x7c00u;
    assert(psx_gpu_gp0_write(&gpu, 0x65000000u));
    assert(psx_gpu_gp0_write(&gpu, 100u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 5u | (6u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 2u | (2u << 16u)));
    assert(pixel(&gpu, 100u, 100u) == 0x001fu);
    assert(pixel(&gpu, 101u, 100u) == 0x03e0u);
    assert(pixel(&gpu, 100u, 101u) == 0x7c00u);

    /* Indexed 4-bit rectangle resolves its CLUT from the UV command word. */
    assert(psx_gpu_gp0_write(&gpu, 0xe1000000u));
    vram[10u * PSX_GPU_VRAM_WIDTH + 2u] = 0x0005u;
    vram[20u * PSX_GPU_VRAM_WIDTH + 21u] = 0x4210u;
    assert(psx_gpu_gp0_write(&gpu, 0x65000000u));
    assert(psx_gpu_gp0_write(&gpu, 120u | (120u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 8u | (10u << 8u) |
                             ((1u | (20u << 6u)) << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 16u)));
    assert(pixel(&gpu, 120u, 120u) == 0x4210u);

    /* Fixed 8x8 raw rectangle uses the E1 texture page and clips normally. */
    assert(psx_gpu_gp0_write(&gpu, 0xe1000000u | (2u << 7u)));
    vram[30u * PSX_GPU_VRAM_WIDTH + 40u] = 0x1234u;
    assert(psx_gpu_gp0_write(&gpu, 0x75000000u));
    assert(psx_gpu_gp0_write(&gpu, 200u | (200u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 40u | (30u << 8u)));
    assert(pixel(&gpu, 200u, 200u) == 0x1234u);
    assert(pixel(&gpu, 207u, 207u) == 0u);


    /* Semi-transparent raw rectangle blends STP texels using E1 mode 0. */
    assert(psx_gpu_gp0_write(&gpu, 0xe1000000u | (2u << 7u)));
    vram[40u * PSX_GPU_VRAM_WIDTH + 50u] = 0x801fu;
    vram[210u * PSX_GPU_VRAM_WIDTH + 210u] = 0x7c00u;
    assert(psx_gpu_gp0_write(&gpu, 0x670000ffu));
    assert(psx_gpu_gp0_write(&gpu, 210u | (210u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 50u | (40u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 16u)));
    assert(pixel(&gpu, 210u, 210u) == 0x3c0fu);


    /* Exercise the remaining E1 semi-transparency blend modes. */
    {
        static const uint16_t expected[3] = {0x7c1fu, 0x7c00u, 0x7c07u};
        for (unsigned mode = 1u; mode < 4u; ++mode) {
            const unsigned dst = 220u + mode;
            assert(psx_gpu_gp0_write(&gpu, 0xe1000000u |
                                     (mode << 5u) | (2u << 7u)));
            vram[40u * PSX_GPU_VRAM_WIDTH + 50u] = 0x801fu;
            vram[dst * PSX_GPU_VRAM_WIDTH + dst] = 0x7c00u;
            assert(psx_gpu_gp0_write(&gpu, 0x670000ffu));
            assert(psx_gpu_gp0_write(&gpu, dst | (dst << 16u)));
            assert(psx_gpu_gp0_write(&gpu, 50u | (40u << 8u)));
            assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 16u)));
            assert(pixel(&gpu, dst, dst) == expected[mode - 1u]);
        }
    }

    /* A texel with STP clear is copied, not blended, by a semi-trans command. */
    assert(psx_gpu_gp0_write(&gpu, 0xe1000000u | (2u << 7u)));
    vram[41u * PSX_GPU_VRAM_WIDTH + 50u] = 0x03e0u;
    vram[211u * PSX_GPU_VRAM_WIDTH + 211u] = 0x7c00u;
    assert(psx_gpu_gp0_write(&gpu, 0x670000ffu));
    assert(psx_gpu_gp0_write(&gpu, 211u | (211u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 50u | (41u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 16u)));
    assert(pixel(&gpu, 211u, 211u) == 0x03e0u);

    /* Modulated fixed-size 8x8 rectangles saturate white texels to white. */
    assert(psx_gpu_gp0_write(&gpu, 0xe1000000u | (2u << 7u)));
    vram[42u * PSX_GPU_VRAM_WIDTH + 60u] = 0x7fffu;
    assert(psx_gpu_gp0_write(&gpu, 0x74ffffffu));
    assert(psx_gpu_gp0_write(&gpu, 230u | (230u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 60u | (42u << 8u)));
    assert(pixel(&gpu, 230u, 230u) == 0x7fffu);

    puts("GPU textured rectangle tests passed");
    return 0;
}
