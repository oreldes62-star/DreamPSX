#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dc/video.h"
#include "dreampsx/kos_video_backend.h"

static uint16_t dreamcast_framebuffer[PSX_KOS_VIDEO_WIDTH *
                                      PSX_KOS_VIDEO_HEIGHT];
uint16_t *vram_s = dreamcast_framebuffer;
static unsigned selected_mode;
static vid_pixel_mode_t selected_pixel_mode;
static unsigned waitvbl_count;
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
    memset(dreamcast_framebuffer, 0, sizeof(dreamcast_framebuffer));
}

void vid_waitvbl(void)
{
    ++waitvbl_count;
}

int main(void)
{
    PsxKosVideoBackend backend;
    PsxGpu gpu;
    static uint16_t psx_vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];

    assert(psx_kos_video_backend_start(&backend));
    assert(backend.initialized);
    assert(selected_mode == DM_320x240);
    assert(selected_pixel_mode == PM_RGB565);
    assert(!dithering_enabled);

    assert(psx_gpu_init(&gpu, psx_vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    assert(psx_gpu_gp1_write(&gpu, 0x08000000u));
    assert(psx_gpu_gp1_write(&gpu, 0x03000000u));
    psx_vram[0] = 0x001fu;
    psx_vram[1] = 0x03e0u;

    assert(psx_kos_video_backend_present(&backend, &gpu));
    assert(backend.presented_frames == 1u);
    assert(waitvbl_count == 1u);
    assert(dreamcast_framebuffer[0] == 0xf800u);
    assert(dreamcast_framebuffer[1] == 0xf800u);
    assert(dreamcast_framebuffer[2] == 0x07e0u);

    assert(!psx_kos_video_backend_present(NULL, &gpu));
    assert(!psx_kos_video_backend_present(&backend, NULL));

    puts("KallistiOS video backend tests passed");
    return 0;
}
