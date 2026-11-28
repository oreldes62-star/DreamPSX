#ifndef DREAMPSX_KOS_VIDEO_BACKEND_H
#define DREAMPSX_KOS_VIDEO_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#include "dreampsx/gpu.h"

#define PSX_KOS_VIDEO_WIDTH  320u
#define PSX_KOS_VIDEO_HEIGHT 240u
#define PSX_KOS_VIDEO_MAX_SOURCE_WIDTH  640u
#define PSX_KOS_VIDEO_MAX_SOURCE_HEIGHT 480u

typedef struct {
    uint16_t scanout[PSX_KOS_VIDEO_MAX_SOURCE_WIDTH *
                     PSX_KOS_VIDEO_MAX_SOURCE_HEIGHT];
    bool initialized;
    uint64_t presented_frames;
} PsxKosVideoBackend;

/* Initialize a 320x240 RGB565 Dreamcast framebuffer. */
bool psx_kos_video_backend_start(PsxKosVideoBackend *backend);
/* Convert the active PS1 GPU display to RGB565, scale it to 320x240,
 * copy it to the KOS framebuffer, and wait for vertical blank.
 */
bool psx_kos_video_backend_present(PsxKosVideoBackend *backend,
                                   const PsxGpu *gpu);

#endif
