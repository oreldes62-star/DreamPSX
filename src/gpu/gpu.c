#include "dreampsx/gpu.h"

#include <string.h>

#define GPU_STATUS_RESET 0x14802000u

static uint16_t rgb24_to_rgb15(uint32_t color)
{
    return (uint16_t)(((color & 0x000000f8u) >> 3u) |
                      ((color & 0x0000f800u) >> 6u) |
                      ((color & 0x00f80000u) >> 9u));
}

static unsigned transfer_extent(uint32_t encoded, unsigned mask,
                                unsigned maximum)
{
    const unsigned extent = (unsigned)(encoded & mask);
    return extent == 0u ? maximum : extent;
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
            gpu->vram[py * PSX_GPU_VRAM_WIDTH + px] = color;
        }
    }
}

static void begin_image_transfer(PsxGpu *gpu, PsxGpuTransferMode mode)
{
    const uint32_t xy = gpu->parameters[0];
    const uint32_t wh = gpu->parameters[1];

    gpu->transfer_x = xy & 0x3ffu;
    gpu->transfer_y = (xy >> 16u) & 0x1ffu;
    gpu->transfer_width = transfer_extent(wh, 0x3ffu,
                                          PSX_GPU_VRAM_WIDTH);
    gpu->transfer_height = transfer_extent(wh >> 16u, 0x1ffu,
                                           PSX_GPU_VRAM_HEIGHT);
    gpu->transfer_pixel_index = 0u;
    gpu->transfer_pixel_count =
        (size_t)gpu->transfer_width * gpu->transfer_height;
    gpu->transfer_mode = mode;
}

static void write_transfer_pixel(PsxGpu *gpu, uint16_t pixel)
{
    const size_t index = gpu->transfer_pixel_index++;
    const unsigned x = (gpu->transfer_x +
                        (unsigned)(index % gpu->transfer_width)) %
                       PSX_GPU_VRAM_WIDTH;
    const unsigned y = (gpu->transfer_y +
                        (unsigned)(index / gpu->transfer_width)) %
                       PSX_GPU_VRAM_HEIGHT;

    gpu->vram[y * PSX_GPU_VRAM_WIDTH + x] = pixel;
    if (gpu->transfer_pixel_index >= gpu->transfer_pixel_count)
        gpu->transfer_mode = PSX_GPU_TRANSFER_IDLE;
}

bool psx_gpu_init(PsxGpu *gpu, uint16_t *vram, size_t vram_pixels)
{
    if (!gpu || !vram ||
        vram_pixels != (size_t)PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT)
        return false;
    memset(gpu, 0, sizeof(*gpu));
    gpu->vram = vram;
    gpu->vram_pixels = vram_pixels;
    memset(gpu->vram, 0, vram_pixels * sizeof(*gpu->vram));
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
    gpu->transfer_mode = PSX_GPU_TRANSFER_IDLE;
    gpu->transfer_x = 0u;
    gpu->transfer_y = 0u;
    gpu->transfer_width = 0u;
    gpu->transfer_height = 0u;
    gpu->transfer_pixel_index = 0u;
    gpu->transfer_pixel_count = 0u;
}

bool psx_gpu_gp0_write(PsxGpu *gpu, uint32_t value)
{
    if (!gpu || !gpu->initialized) return false;

    if (gpu->transfer_mode == PSX_GPU_TRANSFER_CPU_TO_VRAM) {
        write_transfer_pixel(gpu, (uint16_t)value);
        if (gpu->transfer_mode == PSX_GPU_TRANSFER_CPU_TO_VRAM)
            write_transfer_pixel(gpu, (uint16_t)(value >> 16u));
        return true;
    }

    if (gpu->parameter_expected != 0u) {
        gpu->parameters[gpu->parameter_count++] = value;
        if (gpu->parameter_count == gpu->parameter_expected) {
            const unsigned opcode = gpu->command >> 24u;
            if (opcode == 0x02u)
                execute_fill_rectangle(gpu);
            else if (opcode == 0xa0u)
                begin_image_transfer(gpu, PSX_GPU_TRANSFER_CPU_TO_VRAM);
            else if (opcode == 0xc0u)
                begin_image_transfer(gpu, PSX_GPU_TRANSFER_VRAM_TO_CPU);
            gpu->parameter_count = 0u;
            gpu->parameter_expected = 0u;
        }
        return true;
    }

    gpu->command = value;
    switch (value >> 24u) {
    case 0x02u: /* Fill rectangle */
    case 0xa0u: /* Copy image from CPU to VRAM */
    case 0xc0u: /* Copy image from VRAM to CPU */
        gpu->parameter_expected = 2u;
        gpu->parameter_count = 0u;
        break;
    default:
        break;
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
    case 0x01u: /* GP1(01h): reset command buffer and pending transfer. */
        gpu->command = 0u;
        gpu->parameter_count = 0u;
        gpu->parameter_expected = 0u;
        gpu->transfer_mode = PSX_GPU_TRANSFER_IDLE;
        gpu->transfer_pixel_index = 0u;
        gpu->transfer_pixel_count = 0u;
        break;
    default:
        /* Unsupported display/DMA control commands are intentionally inert. */
        break;
    }
    return true;
}

bool psx_gpu_read_data(PsxGpu *gpu, uint32_t *value)
{
    uint16_t low = 0u;
    uint16_t high = 0u;

    if (!gpu || !gpu->initialized || !value) return false;
    if (gpu->transfer_mode == PSX_GPU_TRANSFER_VRAM_TO_CPU) {
        const size_t low_index = gpu->transfer_pixel_index++;
        const unsigned low_x = (gpu->transfer_x +
            (unsigned)(low_index % gpu->transfer_width)) %
            PSX_GPU_VRAM_WIDTH;
        const unsigned low_y = (gpu->transfer_y +
            (unsigned)(low_index / gpu->transfer_width)) %
            PSX_GPU_VRAM_HEIGHT;
        low = gpu->vram[low_y * PSX_GPU_VRAM_WIDTH + low_x];

        if (gpu->transfer_pixel_index < gpu->transfer_pixel_count) {
            const size_t high_index = gpu->transfer_pixel_index++;
            const unsigned high_x = (gpu->transfer_x +
                (unsigned)(high_index % gpu->transfer_width)) %
                PSX_GPU_VRAM_WIDTH;
            const unsigned high_y = (gpu->transfer_y +
                (unsigned)(high_index / gpu->transfer_width)) %
                PSX_GPU_VRAM_HEIGHT;
            high = gpu->vram[high_y * PSX_GPU_VRAM_WIDTH + high_x];
        }

        if (gpu->transfer_pixel_index >= gpu->transfer_pixel_count)
            gpu->transfer_mode = PSX_GPU_TRANSFER_IDLE;
    }
    *value = (uint32_t)low | ((uint32_t)high << 16u);
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
    *value = gpu->vram[y * PSX_GPU_VRAM_WIDTH + x];
    return true;
}
