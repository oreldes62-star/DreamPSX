#ifndef DREAMPSX_GPU_H
#define DREAMPSX_GPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSX_GPU_VRAM_WIDTH  1024u
#define PSX_GPU_VRAM_HEIGHT 512u

typedef enum {
    PSX_GPU_TRANSFER_IDLE = 0,
    PSX_GPU_TRANSFER_CPU_TO_VRAM,
    PSX_GPU_TRANSFER_VRAM_TO_CPU
} PsxGpuTransferMode;

/* VRAM storage is supplied by the owner (1 MiB) to avoid a large machine
 * object or stack allocation on embedded targets.
 */
typedef struct {
    uint16_t *vram;
    size_t vram_pixels;
    uint32_t status;
    uint32_t command;
    uint32_t parameters[8];
    unsigned draw_area_left;
    unsigned draw_area_top;
    unsigned draw_area_right;
    unsigned draw_area_bottom;
    int draw_offset_x;
    int draw_offset_y;
    unsigned draw_mode;
    unsigned texture_window_mask_x;
    unsigned texture_window_mask_y;
    unsigned texture_window_offset_x;
    unsigned texture_window_offset_y;
    unsigned parameter_count;
    unsigned parameter_expected;
    PsxGpuTransferMode transfer_mode;
    unsigned transfer_x;
    unsigned transfer_y;
    unsigned transfer_width;
    unsigned transfer_height;
    size_t transfer_pixel_index;
    size_t transfer_pixel_count;
    bool initialized;
} PsxGpu;

bool psx_gpu_init(PsxGpu *gpu, uint16_t *vram, size_t vram_pixels);
void psx_gpu_reset(PsxGpu *gpu);
bool psx_gpu_gp0_write(PsxGpu *gpu, uint32_t value);
bool psx_gpu_gp1_write(PsxGpu *gpu, uint32_t value);
bool psx_gpu_read_data(PsxGpu *gpu, uint32_t *value);
bool psx_gpu_read_status(const PsxGpu *gpu, uint32_t *value);
bool psx_gpu_vram_read(const PsxGpu *gpu, unsigned x, unsigned y,
                       uint16_t *value);
/* Convert a rectangular RGB555 VRAM region to packed RGB565 output.
 * dst_stride is measured in uint16_t pixels and may exceed width.
 */
bool psx_gpu_copy_rgb565(const PsxGpu *gpu, unsigned x, unsigned y,
                         unsigned width, unsigned height,
                         uint16_t *dst, size_t dst_stride);

#endif
