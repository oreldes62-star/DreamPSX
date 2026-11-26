#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "dreampsx/gpu.h"

int main(void)
{
    PsxGpu gpu;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint16_t pixel = 0xffffu;
    uint32_t status = 0u;

    assert(psx_gpu_init(&gpu, vram, PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    assert(psx_gpu_read_status(&gpu, &status));
    assert(status == 0x14802000u);

    /* GP0(02h): fill a 16-pixel-aligned VRAM rectangle. */
    assert(psx_gpu_gp0_write(&gpu, 0x020000ffu)); /* Red */
    assert(psx_gpu_gp0_write(&gpu, 3u | (4u << 16u))); /* XY */
    assert(psx_gpu_gp0_write(&gpu, 1u | (2u << 16u))); /* WH */
    assert(psx_gpu_vram_read(&gpu, 0u, 4u, &pixel));
    assert(pixel == 0x001fu);
    assert(psx_gpu_vram_read(&gpu, 15u, 5u, &pixel));
    assert(pixel == 0x001fu);
    assert(psx_gpu_vram_read(&gpu, 16u, 4u, &pixel));
    assert(pixel == 0u);
    assert(psx_gpu_vram_read(&gpu, 0u, 6u, &pixel));
    assert(pixel == 0u);

    /* GP1 command-buffer reset discards an incomplete command. */
    assert(psx_gpu_gp0_write(&gpu, 0x0200ff00u));
    assert(psx_gpu_gp0_write(&gpu, 0u));
    assert(psx_gpu_gp1_write(&gpu, 0x01000000u));
    assert(psx_gpu_gp0_write(&gpu, 0u));
    assert(psx_gpu_vram_read(&gpu, 0u, 0u, &pixel));
    assert(pixel == 0u);

    /* A0 uploads an odd number of pixels, low halfword first. */
    assert(psx_gpu_gp0_write(&gpu, 0xa0000000u));
    assert(psx_gpu_gp0_write(&gpu, 10u | (20u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 3u | (1u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x22221111u));
    assert(psx_gpu_gp0_write(&gpu, 0xdead3333u));
    assert(psx_gpu_vram_read(&gpu, 10u, 20u, &pixel) && pixel == 0x1111u);
    assert(psx_gpu_vram_read(&gpu, 11u, 20u, &pixel) && pixel == 0x2222u);
    assert(psx_gpu_vram_read(&gpu, 12u, 20u, &pixel) && pixel == 0x3333u);

    /* C0 reads the same pixels back as packed GPUREAD words. */
    assert(psx_gpu_gp0_write(&gpu, 0xc0000000u));
    assert(psx_gpu_gp0_write(&gpu, 10u | (20u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 3u | (1u << 16u)));
    { uint32_t data = 0u;
      assert(psx_gpu_read_data(&gpu, &data) && data == 0x22221111u);
      assert(psx_gpu_read_data(&gpu, &data) && data == 0x00003333u);
      assert(psx_gpu_read_data(&gpu, &data) && data == 0u); }

    /* GPU reset preserves VRAM but restores status and parser state. */
    assert(psx_gpu_gp1_write(&gpu, 0x00000000u));
    assert(psx_gpu_read_status(&gpu, &status));
    assert(status == 0x14802000u);
    assert(psx_gpu_vram_read(&gpu, 0u, 4u, &pixel));
    assert(pixel == 0x001fu);
    assert(!psx_gpu_vram_read(&gpu, 1024u, 0u, &pixel));

    puts("GPU tests passed");
    return 0;
}
