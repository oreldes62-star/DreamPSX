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

    /* GP0 20h rasterizes a flat-shaded triangle at pixel centers. */
    assert(psx_gpu_gp0_write(&gpu, 0x2000ff00u)); /* Green */
    assert(psx_gpu_gp0_write(&gpu, 100u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 104u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 100u | (104u << 16u)));
    assert(psx_gpu_vram_read(&gpu, 101u, 101u, &pixel));
    assert(pixel == 0x03e0u);
    assert(psx_gpu_vram_read(&gpu, 103u, 103u, &pixel));
    assert(pixel == 0u);

    /* Signed 11-bit coordinates are decoded and clipped at the VRAM edge. */
    assert(psx_gpu_gp0_write(&gpu, 0x200000ffu)); /* Red */
    assert(psx_gpu_gp0_write(&gpu, 0x7feu | (200u << 16u))); /* (-2,200) */
    assert(psx_gpu_gp0_write(&gpu, 2u | (200u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x7feu | (204u << 16u)));
    assert(psx_gpu_vram_read(&gpu, 0u, 200u, &pixel));
    assert(pixel == 0x001fu);

    /* E3/E4 restrict primitive writes to the inclusive drawing area. */
    assert(psx_gpu_gp0_write(&gpu, 0xe3000000u | 201u | (201u << 10u)));
    assert(psx_gpu_gp0_write(&gpu, 0xe4000000u | 202u | (202u << 10u)));
    assert(psx_gpu_gp0_write(&gpu, 0x200000ffu));
    assert(psx_gpu_gp0_write(&gpu, 200u | (200u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 204u | (200u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 200u | (204u << 16u)));
    assert(psx_gpu_vram_read(&gpu, 201u, 201u, &pixel));
    assert(pixel == 0x001fu);
    assert(psx_gpu_vram_read(&gpu, 200u, 200u, &pixel));
    assert(pixel == 0u);

    /* E5 signed offsets translate all three submitted vertices. */
    assert(psx_gpu_gp0_write(&gpu, 0xe3000000u));
    assert(psx_gpu_gp0_write(&gpu, 0xe4000000u | 1023u | (511u << 10u)));
    assert(psx_gpu_gp0_write(&gpu, 0xe5000000u | 10u | (20u << 11u)));
    assert(psx_gpu_gp0_write(&gpu, 0x2000ff00u));
    assert(psx_gpu_gp0_write(&gpu, 100u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 104u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 100u | (104u << 16u)));
    assert(psx_gpu_vram_read(&gpu, 111u, 121u, &pixel));
    assert(pixel == 0x03e0u);
    assert(psx_gpu_vram_read(&gpu, 101u, 101u, &pixel));
    assert(pixel == 0x03e0u); /* Existing primitive remains untouched. */

    /* Shared diagonal pixels belong to exactly one triangle. */
    assert(psx_gpu_gp0_write(&gpu, 0xe5000000u));
    assert(psx_gpu_gp0_write(&gpu, 0x200000ffu)); /* First triangle red. */
    assert(psx_gpu_gp0_write(&gpu, 300u | (300u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 304u | (300u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 300u | (304u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x2000ff00u)); /* Second triangle green. */
    assert(psx_gpu_gp0_write(&gpu, 304u | (300u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 304u | (304u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 300u | (304u << 16u)));
    assert(psx_gpu_vram_read(&gpu, 302u, 301u, &pixel));
    assert(pixel == 0x03e0u);

    /* GP0 30h interpolates the three vertex colors across a triangle. */
    assert(psx_gpu_gp0_write(&gpu, 0x300000ffu)); /* Vertex 0 red. */
    assert(psx_gpu_gp0_write(&gpu, 400u | (400u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x0000ff00u)); /* Vertex 1 green. */
    assert(psx_gpu_gp0_write(&gpu, 404u | (400u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x00ff0000u)); /* Vertex 2 blue. */
    assert(psx_gpu_gp0_write(&gpu, 400u | (404u << 16u)));
    assert(psx_gpu_vram_read(&gpu, 401u, 401u, &pixel));
    assert(pixel == 0x2d67u);

    /* GP0 25h samples a direct 15-bit texture page. */
    vram[2u * PSX_GPU_VRAM_WIDTH + 2u] = 0x7c00u;
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 500u | (500u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 504u | (500u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 5u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 500u | (504u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (5u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 501u, 501u, &pixel));
    assert(pixel == 0x7c00u);

    /* Transparent zero texels leave the destination pixel untouched. */
    vram[10u * PSX_GPU_VRAM_WIDTH + 10u] = 0u;
    vram[451u * PSX_GPU_VRAM_WIDTH + 601u] = 0x1234u;
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 600u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 10u | (10u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 604u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 10u | (10u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 600u | (454u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 10u | (10u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 601u, 451u, &pixel));
    assert(pixel == 0x1234u);

    /* 4-bit indexed texture: packed nibble is resolved through CLUT. */
    vram[20u * PSX_GPU_VRAM_WIDTH] = 0x0003u;
    vram[30u * PSX_GPU_VRAM_WIDTH + 3u] = 0x4210u;
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 650u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, (30u << 22u) | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 654u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 650u | (454u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 651u, 451u, &pixel));
    assert(pixel == 0x4210u);

    /* 8-bit indexed texture: byte index is resolved through CLUT. */
    vram[40u * PSX_GPU_VRAM_WIDTH] = 0x0005u;
    vram[50u * PSX_GPU_VRAM_WIDTH + 5u] = 0x6318u;
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 700u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, (50u << 22u) | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 704u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x00800000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 700u | (454u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 701u, 451u, &pixel));
    assert(pixel == 0x6318u);

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
