#ifndef DREAMPSX_GPU_H
#define DREAMPSX_GPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSX_GPU_VRAM_WIDTH  1024u
#define PSX_GPU_VRAM_HEIGHT 512u

typedef struct {
    unsigned width;
    unsigned height;
    bool pal;
    bool color_24bit;
    bool interlaced;
    bool reverse;
    unsigned range_width;
    unsigned range_height;
    bool range_valid;
    bool field_odd;
    bool field_valid;
} PsxGpuDisplayModeInfo;

typedef struct {
    unsigned scanline;
    unsigned lines_per_field;
    uint64_t frame_counter;
    bool pal;
    bool interlaced;
    bool field_odd;
    bool vblank;
    bool hblank_pulse;
    bool field_started;
    bool frame_started;
    bool vblank_started;
    bool vblank_ended;
} PsxGpuVideoState;

typedef struct {
    unsigned start_x;
    unsigned start_y;
    unsigned horizontal_start;
    unsigned horizontal_end;
    unsigned vertical_start;
    unsigned vertical_end;
    unsigned mode;
    bool disabled;
    bool field_odd;
    unsigned video_scanline;
    uint64_t video_frame_counter;
    bool video_vblank;
} PsxGpuDisplay;

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
    unsigned dma_direction;
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
    PsxGpuDisplay display;
    bool initialized;
} PsxGpu;

bool psx_gpu_init(PsxGpu *gpu, uint16_t *vram, size_t vram_pixels);
void psx_gpu_reset(PsxGpu *gpu);
bool psx_gpu_gp0_write(PsxGpu *gpu, uint32_t value);
bool psx_gpu_gp1_write(PsxGpu *gpu, uint32_t value);
bool psx_gpu_read_data(PsxGpu *gpu, uint32_t *value);
bool psx_gpu_read_status(const PsxGpu *gpu, uint32_t *value);
/* GPU IRQ1 output, asserted by GP0(1Fh) until GP1(02h) acknowledges it. */
bool psx_gpu_irq_output(const PsxGpu *gpu);
bool psx_gpu_get_display(const PsxGpu *gpu, PsxGpuDisplay *display);
bool psx_gpu_get_display_mode_info(const PsxGpu *gpu,
                                  PsxGpuDisplayModeInfo *info);
/* Set the externally clocked odd/even field for 480-line interlace.
 * Returns false unless 480-line interlace is selected.
 */
bool psx_gpu_set_display_field(PsxGpu *gpu, bool odd_field);
/* Advance the raster by one scanline and report one abstract HBlank pulse.
 * This pulse is line-based; horizontal phase and exact dot timing are not
 * modeled yet. Interlaced 480-line output alternates
 * integer field lengths (262/263 NTSC, 312/313 PAL); half-line phase is
 * intentionally outside this first scheduler API.
 */
bool psx_gpu_video_advance_scanline(PsxGpu *gpu,
                                    PsxGpuVideoState *state);
bool psx_gpu_vram_read(const PsxGpu *gpu, unsigned x, unsigned y,
                       uint16_t *value);
/* Convert a rectangular RGB555 VRAM region to packed RGB565 output.
 * dst_stride is measured in uint16_t pixels and may exceed width.
 */
bool psx_gpu_copy_rgb565(const PsxGpu *gpu, unsigned x, unsigned y,
                         unsigned width, unsigned height,
                         uint16_t *dst, size_t dst_stride);
/* Read the configured display origin into RGB565 output. Supports PS1
 * 15-bit pixels and byte-packed 24-bit pixels; width/height and destination
 * stride are supplied by the frontend.
 */
bool psx_gpu_scanout_rgb565(const PsxGpu *gpu, unsigned width,
                            unsigned height, uint16_t *dst,
                            size_t dst_stride);

#endif
