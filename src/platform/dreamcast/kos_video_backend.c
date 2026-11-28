#include "dreampsx/kos_video_backend.h"

#include <dc/video.h>

#include <stddef.h>
#include <string.h>

bool psx_kos_video_backend_start(PsxKosVideoBackend *backend)
{
    if (!backend) return false;

    memset(backend, 0, sizeof(*backend));
    vid_set_mode(DM_320x240, PM_RGB565);
    vid_set_dithering(false);
    if (!vram_s) return false;

    vid_clear(0u, 0u, 0u);
    backend->initialized = true;
    return true;
}

bool psx_kos_video_backend_present(PsxKosVideoBackend *backend,
                                   const PsxGpu *gpu)
{
    PsxGpuDisplayModeInfo info;
    uint16_t *framebuffer;
    unsigned source_width;
    unsigned source_height;

    if (!backend || !backend->initialized || !gpu ||
        !psx_gpu_get_display_mode_info(gpu, &info))
        return false;

    /* GP1(06h/07h) define the active display window. Prefer those measured
       dimensions when programmed; fall back to the selected video mode for
       software that leaves the range registers at their reset values. */
    source_width = info.range_valid ? info.range_width : info.width;
    source_height = info.range_valid ? info.range_height : info.height;
    if (source_width == 0u || source_height == 0u ||
        source_width > PSX_KOS_VIDEO_MAX_SOURCE_WIDTH ||
        source_height > PSX_KOS_VIDEO_MAX_SOURCE_HEIGHT)
        return false;

    if (!psx_gpu_scanout_rgb565(gpu, source_width, source_height,
                                backend->scanout, source_width))
        return false;

    framebuffer = vram_s;
    if (!framebuffer) return false;

    /* Nearest-neighbour scaling keeps the frontend deterministic and avoids
       floating-point work on the SH-4. */
    for (unsigned y = 0u; y < PSX_KOS_VIDEO_HEIGHT; ++y) {
        const unsigned source_y =
            (unsigned)(((uint64_t)y * source_height) /
                       PSX_KOS_VIDEO_HEIGHT);
        for (unsigned x = 0u; x < PSX_KOS_VIDEO_WIDTH; ++x) {
            const unsigned source_x =
                (unsigned)(((uint64_t)x * source_width) /
                           PSX_KOS_VIDEO_WIDTH);
            framebuffer[y * PSX_KOS_VIDEO_WIDTH + x] =
                backend->scanout[source_y * source_width + source_x];
        }
    }

    vid_waitvbl();
    ++backend->presented_frames;
    return true;
}
