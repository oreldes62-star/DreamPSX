#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "dreampsx/gpu.h"

int main(void)
{
    PsxGpu gpu;
    uint16_t pixel = 0xffffu;
    uint32_t status = 0u;

    assert(psx_gpu_init(&gpu));
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
