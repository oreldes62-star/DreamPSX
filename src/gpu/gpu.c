#include "dreampsx/gpu.h"

#include <string.h>

#define GPU_STATUS_RESET 0x14802000u

static uint16_t rgb24_to_rgb15(uint32_t color)
{
    return (uint16_t)(((color & 0x000000f8u) >> 3u) |
                      ((color & 0x0000f800u) >> 6u) |
                      ((color & 0x00f80000u) >> 9u));
}

static void execute_fill_rectangle(PsxGpu *gpu)
{
    const uint32_t xy = gpu->parameters[0];
    const uint32_t wh = gpu->parameters[1];
    const unsigned x = (xy & 0x3ffu) & ~15u;
    const unsigned y = (xy >> 16u) & 0x1ffu;
    const unsigned width = ((wh & 0x3ffu) + 15u) & ~15u;
    const unsigned height = (wh >> 16u) & 0x1ffu;
    const uint16_t color = rgb24_to_rgb15(gpu->command);

    for (unsigned row = 0; row < height; ++row) {
        const unsigned py = y + row;
        if (py >= PSX_GPU_VRAM_HEIGHT) break;
        for (unsigned column = 0; column < width; ++column) {
            const unsigned px = x + column;
            if (px >= PSX_GPU_VRAM_WIDTH) break;
            gpu->vram[py][px] = color;
        }
    }
}

bool psx_gpu_init(PsxGpu *gpu)
{
    if (!gpu) return false;
    memset(gpu, 0, sizeof(*gpu));
    gpu->initialized = true;
    psx_gpu_reset(gpu);
    return true;
}

void psx_gpu_reset(PsxGpu *gpu)
{
    if (!gpu) return;
    gpu->status = GPU_STATUS_RESET;
    gpu->command = 0u;
    gpu->parameters[0] = 0u;
    gpu->parameters[1] = 0u;
    gpu->parameter_count = 0u;
    gpu->parameter_expected = 0u;
}

bool psx_gpu_gp0_write(PsxGpu *gpu, uint32_t value)
{
    if (!gpu || !gpu->initialized) return false;

    if (gpu->parameter_expected != 0u) {
        gpu->parameters[gpu->parameter_count++] = value;
        if (gpu->parameter_count == gpu->parameter_expected) {
            if ((gpu->command >> 24u) == 0x02u)
                execute_fill_rectangle(gpu);
            gpu->parameter_count = 0u;
            gpu->parameter_expected = 0u;
        }
        return true;
    }

    gpu->command = value;
    if ((value >> 24u) == 0x02u) {
        gpu->parameter_expected = 2u;
        gpu->parameter_count = 0u;
    }
    return true;
}

bool psx_gpu_gp1_write(PsxGpu *gpu, uint32_t value)
{
    if (!gpu || !gpu->initialized) return false;
    switch (value >> 24u) {
    case 0x00u: /* GP1(00h): GPU reset. */
        psx_gpu_reset(gpu);
        break;
    case 0x01u: /* GP1(01h): reset command buffer. */
        gpu->command = 0u;
        gpu->parameter_count = 0u;
        gpu->parameter_expected = 0u;
        break;
    default:
        /* Unsupported display/DMA control commands are intentionally inert. */
        break;
    }
    return true;
}

bool psx_gpu_read_status(const PsxGpu *gpu, uint32_t *value)
{
    if (!gpu || !gpu->initialized || !value) return false;
    *value = gpu->status;
    return true;
}

bool psx_gpu_vram_read(const PsxGpu *gpu, unsigned x, unsigned y,
                       uint16_t *value)
{
    if (!gpu || !gpu->initialized || !value ||
        x >= PSX_GPU_VRAM_WIDTH || y >= PSX_GPU_VRAM_HEIGHT)
        return false;
    *value = gpu->vram[y][x];
    return true;
}
