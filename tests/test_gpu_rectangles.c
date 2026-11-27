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

    /* Variable-size rectangle obeys the inclusive drawing-area clip. */
    vram[100u * PSX_GPU_VRAM_WIDTH + 100u] = 0x1234u;
    vram[101u * PSX_GPU_VRAM_WIDTH + 103u] = 0x2345u;
    assert(psx_gpu_gp0_write(&gpu, 0xe3000000u | 101u | (101u << 10u)));
    assert(psx_gpu_gp0_write(&gpu, 0xe4000000u | 102u | (102u << 10u)));
    assert(psx_gpu_gp0_write(&gpu, 0x600000ffu));
    assert(psx_gpu_gp0_write(&gpu, 100u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 4u | (3u << 16u)));
    assert(pixel(&gpu, 101u, 101u) == 0x001fu);
    assert(pixel(&gpu, 102u, 102u) == 0x001fu);
    assert(pixel(&gpu, 100u, 100u) == 0x1234u);
    assert(pixel(&gpu, 103u, 101u) == 0x2345u);

    /* Fixed-size 1x1, 8x8 and 16x16 rectangle command families. */
    assert(psx_gpu_gp0_write(&gpu, 0xe3000000u));
    assert(psx_gpu_gp0_write(&gpu, 0xe4000000u | 1023u | (511u << 10u)));
    assert(psx_gpu_gp0_write(&gpu, 0x6800ff00u));
    assert(psx_gpu_gp0_write(&gpu, 300u | (300u << 16u)));
    assert(pixel(&gpu, 300u, 300u) == 0x03e0u);
    assert(pixel(&gpu, 301u, 300u) == 0u);

    assert(psx_gpu_gp0_write(&gpu, 0x700000ffu));
    assert(psx_gpu_gp0_write(&gpu, 320u | (320u << 16u)));
    assert(pixel(&gpu, 327u, 327u) == 0x001fu);
    assert(pixel(&gpu, 328u, 327u) == 0u);

    assert(psx_gpu_gp0_write(&gpu, 0x780000ffu));
    assert(psx_gpu_gp0_write(&gpu, 340u | (340u << 16u)));
    assert(pixel(&gpu, 355u, 355u) == 0x001fu);
    assert(pixel(&gpu, 356u, 355u) == 0u);

    /* Signed draw offset is applied before clipping and rasterization. */
    assert(psx_gpu_gp0_write(&gpu, 0xe5000000u | 0x7feu | (0x7ffu << 11u)));
    assert(psx_gpu_gp0_write(&gpu, 0x680000ffu));
    assert(psx_gpu_gp0_write(&gpu, 400u | (400u << 16u)));
    assert(pixel(&gpu, 398u, 399u) == 0x001fu);
    assert(pixel(&gpu, 400u, 400u) == 0u);

    puts("GPU monochrome rectangle tests passed");
    return 0;
}
