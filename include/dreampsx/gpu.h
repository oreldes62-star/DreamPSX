#ifndef DREAMPSX_GPU_H
#define DREAMPSX_GPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSX_GPU_VRAM_WIDTH  1024u
#define PSX_GPU_VRAM_HEIGHT 512u

typedef struct {
    uint16_t *vram;
    size_t vram_pixels;
    uint32_t status;
    uint32_t command;
    uint32_t parameters[2];
    unsigned parameter_count;
    unsigned parameter_expected;
    bool initialized;
} PsxGpu;

bool psx_gpu_init(PsxGpu *gpu, uint16_t *vram, size_t vram_pixels);
void psx_gpu_reset(PsxGpu *gpu);
bool psx_gpu_gp0_write(PsxGpu *gpu, uint32_t value);
bool psx_gpu_gp1_write(PsxGpu *gpu, uint32_t value);
bool psx_gpu_read_status(const PsxGpu *gpu, uint32_t *value);
bool psx_gpu_vram_read(const PsxGpu *gpu, unsigned x, unsigned y,
                       uint16_t *value);

#endif
