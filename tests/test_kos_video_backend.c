#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dc/video.h"
#include "dreampsx/kos_video_backend.h"

static uint16_t framebuffer_a[PSX_KOS_VIDEO_WIDTH * PSX_KOS_VIDEO_HEIGHT];
static uint16_t framebuffer_b[PSX_KOS_VIDEO_WIDTH * PSX_KOS_VIDEO_HEIGHT];
uint16_t *vram_s = framebuffer_a;
static uint16_t *displayed_framebuffer;
static unsigned selected_mode;
static vid_pixel_mode_t selected_pixel_mode;
static unsigned waitvbl_count;
static unsigned flip_count;
static bool dithering_enabled;

void vid_set_mode(int mode, vid_pixel_mode_t pixel_mode)
{
    selected_mode = (unsigned)mode;
    selected_pixel_mode = pixel_mode;
}

void vid_set_dithering(bool enable)
{
    dithering_enabled = enable;
}

void vid_clear(uint8_t r, uint8_t g, uint8_t b)
{
    (void)r;
    (void)g;
    (void)b;
    assert(vram_s != NULL);
    memset(vram_s, 0, PSX_KOS_VIDEO_WIDTH * PSX_KOS_VIDEO_HEIGHT *
                     sizeof(*vram_s));
}

void vid_waitvbl(void)
{
    ++waitvbl_count;
}

void vid_flip(int32_t framebuffer)
{
    assert(framebuffer == -1);
    displayed_framebuffer = vram_s;
    vram_s = (vram_s == framebuffer_a) ? framebuffer_b : framebuffer_a;
    ++flip_count;
}

int main(void)
{
    PsxKosVideoBackend backend;
    PsxGpu gpu;
    static uint16_t psx_vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];

    assert(psx_kos_video_backend_start(&backend));
    assert(backend.initialized);
    assert(selected_mode == (DM_320x240 | DM_MULTIBUFFER));
    assert(selected_pixel_mode == PM_RGB565);
    assert(!dithering_enabled);
    assert(waitvbl_count == 1u);
    assert(flip_count == 1u);
    assert(displayed_framebuffer == framebuffer_a);
    assert(vram_s == framebuffer_b);

    assert(psx_gpu_init(&gpu, psx_vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    assert(psx_gpu_gp1_write(&gpu, 0x08000000u));
    assert(psx_gpu_gp1_write(&gpu, 0x03000000u));
    psx_vram[0] = 0x001fu;
    psx_vram[1] = 0x03e0u;

    assert(psx_kos_video_backend_present(&backend, &gpu));
    assert(backend.presented_frames == 1u);
    assert(waitvbl_count == 2u);
    assert(flip_count == 2u);
    assert(displayed_framebuffer == framebuffer_b);
    assert(framebuffer_b[0] == 0xf800u);
    assert(framebuffer_b[1] == 0xf800u);
    assert(framebuffer_b[2] == 0x07e0u);
    assert(vram_s == framebuffer_a);

    /* A 320-pixel active range must be used instead of the reset 256-pixel
       mode width, otherwise the right edge is sampled from the wrong texel. */
    assert(psx_gpu_gp1_write(&gpu, 0x08000001u));
    assert(psx_gpu_gp1_write(&gpu, 0x06a00000u));
    assert(psx_gpu_gp1_write(&gpu, 0x0703c000u));
    psx_vram[319] = 0x03e0u;
    assert(psx_kos_video_backend_present(&backend, &gpu));
    assert(backend.presented_frames == 2u);
    assert(waitvbl_count == 3u);
    assert(flip_count == 3u);
    assert(displayed_framebuffer == framebuffer_a);
    assert(framebuffer_a[PSX_KOS_VIDEO_WIDTH - 1u] == 0x07e0u);
    assert(vram_s == framebuffer_b);

    assert(!psx_kos_video_backend_present(NULL, &gpu));
    assert(!psx_kos_video_backend_present(&backend, NULL));

    puts("KallistiOS video backend tests passed");
    return 0;
}
