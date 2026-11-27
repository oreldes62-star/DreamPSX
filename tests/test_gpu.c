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

    /* 24h modulates a white texel by the command's red vertex color. */
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0x7fffu;
    assert(psx_gpu_gp0_write(&gpu, 0x240000ffu));
    assert(psx_gpu_gp0_write(&gpu, 750u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 754u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 750u | (454u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 751u, 451u, &pixel));
    assert(pixel == 0x001fu);

    /* Semi-transparency mode 0: average background and foreground. */
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0xffffu;
    vram[461u * PSX_GPU_VRAM_WIDTH + 461u] = 0u;
    assert(psx_gpu_gp0_write(&gpu, 0xe1000000u));
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 460u | (460u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 804u | (460u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 460u | (804u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 461u, 461u, &pixel));
    assert(pixel == 0x3defu);

    /* Semi-transparency mode 1: saturated addition. */
    vram[471u * PSX_GPU_VRAM_WIDTH + 461u] = 0u;
    assert(psx_gpu_gp0_write(&gpu, 0xe1000020u));
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 460u | (470u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 804u | (470u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 460u | (814u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 461u, 471u, &pixel));
    assert(pixel == 0x7fffu);

    /* Semi-transparency mode 2: background minus foreground. */
    vram[481u * PSX_GPU_VRAM_WIDTH + 461u] = 0x7fffu;
    assert(psx_gpu_gp0_write(&gpu, 0xe1000040u));
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 460u | (480u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 804u | (480u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 460u | (824u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 461u, 481u, &pixel));
    assert(pixel == 0u);

    /* Semi-transparency mode 3: quarter foreground plus background. */
    vram[491u * PSX_GPU_VRAM_WIDTH + 461u] = 0u;
    assert(psx_gpu_gp0_write(&gpu, 0xe1000060u));
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 460u | (490u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 804u | (490u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 460u | (834u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 461u, 491u, &pixel));
    assert(pixel == 0x1ce7u);

    /* 4-bit indexed texture: packed nibble is resolved through CLUT. */
    vram[257u * PSX_GPU_VRAM_WIDTH] = 0x0030u;
    vram[30u * PSX_GPU_VRAM_WIDTH + 3u] = 0x4210u;
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 650u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, (30u << 22u) | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 654u | (450u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x00100000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 650u | (454u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 651u, 451u, &pixel));
    assert(pixel == 0x4210u);

    /* 8-bit indexed texture: byte index is resolved through CLUT. */
    vram[1u * PSX_GPU_VRAM_WIDTH] = 0x0500u;
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


    /* GP0(E2h) remaps direct 15-bit texture coordinates before sampling. */
    vram[9u * PSX_GPU_VRAM_WIDTH + 9u] = 0x4210u;
    assert(psx_gpu_gp0_write(&gpu, 0xe2000000u | 1u | (1u << 5u) |
                             (1u << 10u) | (1u << 15u)));
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 800u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 804u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 800u | (104u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 801u, 101u, &pixel));
    assert(pixel == 0x4210u);

    /* The same transform is applied before indexed 4-bit CLUT lookup. */
    vram[265u * PSX_GPU_VRAM_WIDTH + 2u] = 0x0030u;
    vram[30u * PSX_GPU_VRAM_WIDTH + 3u] = 0x4210u;
    assert(psx_gpu_gp0_write(&gpu, 0xe2000000u | 1u | (1u << 5u) |
                             (1u << 10u) | (1u << 15u)));
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 820u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, (30u << 22u) | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 824u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x00100000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 820u | (104u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 821u, 101u, &pixel));
    assert(pixel == 0x4210u);

    /* 8-bit indexed textures use the same remapped U/V coordinates. */
    vram[265u * PSX_GPU_VRAM_WIDTH + 4u] = 0x0500u;
    vram[50u * PSX_GPU_VRAM_WIDTH + 5u] = 0x6318u;
    assert(psx_gpu_gp0_write(&gpu, 0xe2000000u | 1u | (1u << 5u) |
                             (1u << 10u) | (1u << 15u)));
    assert(psx_gpu_gp0_write(&gpu, 0x25000000u));
    assert(psx_gpu_gp0_write(&gpu, 840u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, (50u << 22u) | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 844u | (100u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x00900000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 840u | (104u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 841u, 101u, &pixel));
    assert(pixel == 0x6318u);

    /* GP0(34h) interpolates vertex colors while sampling a textured triangle. */
    /* Clear the texture window left enabled by the preceding E2h tests. */
    assert(psx_gpu_gp0_write(&gpu, 0xe2000000u));
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0x7fffu;
    assert(psx_gpu_gp0_write(&gpu, 0x340000ffu)); /* Red vertex 0. */
    assert(psx_gpu_gp0_write(&gpu, 900u | (300u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 0x0000ff00u)); /* Green vertex 1. */
    assert(psx_gpu_gp0_write(&gpu, 904u | (300u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 0x00ff0000u)); /* Blue vertex 2. */
    assert(psx_gpu_gp0_write(&gpu, 900u | (304u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 901u, 300u, &pixel));
    assert(pixel == 0x1edeu);

    /* GP0(36h) combines Gouraud modulation with STP-controlled blending. */
    vram[1u * PSX_GPU_VRAM_WIDTH + 1u] = 0xffffu;
    vram[461u * PSX_GPU_VRAM_WIDTH + 961u] = 0x001fu;
    assert(psx_gpu_gp0_write(&gpu, 0xe1000000u)); /* Average blend mode. */
    assert(psx_gpu_gp0_write(&gpu, 0x36808080u)); /* White vertex 0. */
    assert(psx_gpu_gp0_write(&gpu, 960u | (460u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 0x00808080u)); /* White vertex 1. */
    assert(psx_gpu_gp0_write(&gpu, 964u | (460u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 0x01000000u | 1u | (1u << 8u)));
    assert(psx_gpu_gp0_write(&gpu, 0x00808080u)); /* White vertex 2. */
    assert(psx_gpu_gp0_write(&gpu, 960u | (464u << 16u)));
    assert(psx_gpu_gp0_write(&gpu, 1u | (1u << 8u)));
    assert(psx_gpu_vram_read(&gpu, 961u, 461u, &pixel));
    assert(pixel == 0x3defu); /* (red background + white texel) / 2. */

    /* GPU reset clears texture-window state. */
    assert(psx_gpu_gp1_write(&gpu, 0x00000000u));
    assert(gpu.texture_window_mask_x == 0u);
    assert(gpu.texture_window_mask_y == 0u);
    assert(gpu.texture_window_offset_x == 0u);
    assert(gpu.texture_window_offset_y == 0u);

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
