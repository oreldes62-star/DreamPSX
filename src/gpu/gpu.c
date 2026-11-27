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


static int sign_extend_11(uint32_t value)
{
    const unsigned bits = value & 0x7ffu;
    return (bits & 0x400u) != 0u ? (int)bits - 0x800 : (int)bits;
}

static int64_t triangle_edge(int ax, int ay, int bx, int by,
                             int px2, int py2)
{
    return (int64_t)(px2 - 2 * ax) * (2 * by - 2 * ay) -
           (int64_t)(py2 - 2 * ay) * (2 * bx - 2 * ax);
}

static bool triangle_edge_is_top_left(int ax, int ay, int bx, int by,
                                      bool reverse)
{
    const int dx = reverse ? ax - bx : bx - ax;
    const int dy = reverse ? ay - by : by - ay;
    return dy < 0 || (dy == 0 && dx > 0);
}

static bool triangle_edge_inside(int64_t edge, int ax, int ay, int bx, int by,
                                 bool reverse, bool positive_area)
{
    if (positive_area)
        return edge > 0 || (edge == 0 &&
            triangle_edge_is_top_left(ax, ay, bx, by, reverse));
    return edge < 0 || (edge == 0 &&
        triangle_edge_is_top_left(ax, ay, bx, by, reverse));
}

static unsigned interpolate_color_channel(unsigned c0, unsigned c1,
                                                   unsigned c2, int64_t e0,
                                                   int64_t e1, int64_t e2,
                                                   int64_t area)
{
    const int64_t weighted = (int64_t)c0 * e1 +
                             (int64_t)c1 * e2 +
                             (int64_t)c2 * e0;
    int64_t result = weighted / area;
    if (result < 0) result = 0;
    if (result > 255) result = 255;
    return (unsigned)result;
}

static void execute_gouraud_triangle(PsxGpu *gpu)
{
    const uint32_t color0 = gpu->command;
    const uint32_t xy0 = gpu->parameters[0];
    const uint32_t color1 = gpu->parameters[1];
    const uint32_t xy1 = gpu->parameters[2];
    const uint32_t color2 = gpu->parameters[3];
    const uint32_t xy2 = gpu->parameters[4];
    const int x0 = sign_extend_11(xy0) + gpu->draw_offset_x;
    const int y0 = sign_extend_11(xy0 >> 16u) + gpu->draw_offset_y;
    const int x1 = sign_extend_11(xy1) + gpu->draw_offset_x;
    const int y1 = sign_extend_11(xy1 >> 16u) + gpu->draw_offset_y;
    const int x2 = sign_extend_11(xy2) + gpu->draw_offset_x;
    const int y2 = sign_extend_11(xy2 >> 16u) + gpu->draw_offset_y;
    const int min_x = x0 < x1 ? (x0 < x2 ? x0 : x2) :
                                (x1 < x2 ? x1 : x2);
    const int max_x = x0 > x1 ? (x0 > x2 ? x0 : x2) :
                                (x1 > x2 ? x1 : x2);
    const int min_y = y0 < y1 ? (y0 < y2 ? y0 : y2) :
                                (y1 < y2 ? y1 : y2);
    const int max_y = y0 > y1 ? (y0 > y2 ? y0 : y2) :
                                (y1 > y2 ? y1 : y2);
    const int64_t area = triangle_edge(x0, y0, x1, y1, 2 * x2, 2 * y2);
    const bool reverse = area > 0;
    int start_x, end_x, start_y, end_y;

    if (area == 0 || max_x < (int)gpu->draw_area_left ||
        max_y < (int)gpu->draw_area_top ||
        min_x > (int)gpu->draw_area_right ||
        min_y > (int)gpu->draw_area_bottom)
        return;

    start_x = min_x < (int)gpu->draw_area_left ?
              (int)gpu->draw_area_left : min_x;
    end_x = max_x > (int)gpu->draw_area_right ?
            (int)gpu->draw_area_right : max_x;
    start_y = min_y < (int)gpu->draw_area_top ?
              (int)gpu->draw_area_top : min_y;
    end_y = max_y > (int)gpu->draw_area_bottom ?
            (int)gpu->draw_area_bottom : max_y;

    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            const int px2 = 2 * x + 1;
            const int py2 = 2 * y + 1;
            const int64_t e0 = triangle_edge(x0, y0, x1, y1, px2, py2);
            const int64_t e1 = triangle_edge(x1, y1, x2, y2, px2, py2);
            const int64_t e2 = triangle_edge(x2, y2, x0, y0, px2, py2);
            const bool inside =
                triangle_edge_inside(e0, x0, y0, x1, y1,
                                     reverse, area > 0) &&
                triangle_edge_inside(e1, x1, y1, x2, y2,
                                     reverse, area > 0) &&
                triangle_edge_inside(e2, x2, y2, x0, y0,
                                     reverse, area > 0);
            if (inside) {
                const unsigned red = interpolate_color_channel(
                    color0 & 0xffu, color1 & 0xffu, color2 & 0xffu,
                    e0, e1, e2, area);
                const unsigned green = interpolate_color_channel(
                    (color0 >> 8u) & 0xffu, (color1 >> 8u) & 0xffu,
                    (color2 >> 8u) & 0xffu, e0, e1, e2, area);
                const unsigned blue = interpolate_color_channel(
                    (color0 >> 16u) & 0xffu, (color1 >> 16u) & 0xffu,
                    (color2 >> 16u) & 0xffu, e0, e1, e2, area);
                const uint32_t color = red | (green << 8u) | (blue << 16u);
                gpu->vram[(unsigned)y * PSX_GPU_VRAM_WIDTH +
                          (unsigned)x] = rgb24_to_rgb15(color);
            }
        }
    }
}

static void execute_flat_triangle(PsxGpu *gpu)
{
    const uint32_t xy0 = gpu->parameters[0];
    const uint32_t xy1 = gpu->parameters[1];
    const uint32_t xy2 = gpu->parameters[2];
    const int x0 = sign_extend_11(xy0) + gpu->draw_offset_x;
    const int y0 = sign_extend_11(xy0 >> 16u) + gpu->draw_offset_y;
    const int x1 = sign_extend_11(xy1) + gpu->draw_offset_x;
    const int y1 = sign_extend_11(xy1 >> 16u) + gpu->draw_offset_y;
    const int x2 = sign_extend_11(xy2) + gpu->draw_offset_x;
    const int y2 = sign_extend_11(xy2 >> 16u) + gpu->draw_offset_y;
    const int min_x = (x0 < x1 ? (x0 < x2 ? x0 : x2) :
                                      (x1 < x2 ? x1 : x2));
    const int max_x = (x0 > x1 ? (x0 > x2 ? x0 : x2) :
                                      (x1 > x2 ? x1 : x2));
    const int min_y = (y0 < y1 ? (y0 < y2 ? y0 : y2) :
                                      (y1 < y2 ? y1 : y2));
    const int max_y = (y0 > y1 ? (y0 > y2 ? y0 : y2) :
                                      (y1 > y2 ? y1 : y2));
    const int64_t area = triangle_edge(x0, y0, x1, y1, 2 * x2, 2 * y2);
    const uint16_t color = rgb24_to_rgb15(gpu->command);
    int start_x, end_x, start_y, end_y;

    if (area == 0 || max_x < (int)gpu->draw_area_left ||
        max_y < (int)gpu->draw_area_top ||
        min_x > (int)gpu->draw_area_right ||
        min_y > (int)gpu->draw_area_bottom)
        return;

    start_x = min_x < (int)gpu->draw_area_left ?
              (int)gpu->draw_area_left : min_x;
    end_x = max_x > (int)gpu->draw_area_right ?
            (int)gpu->draw_area_right : max_x;
    start_y = min_y < (int)gpu->draw_area_top ?
              (int)gpu->draw_area_top : min_y;
    end_y = max_y > (int)gpu->draw_area_bottom ?
            (int)gpu->draw_area_bottom : max_y;

    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            const int px2 = 2 * x + 1;
            const int py2 = 2 * y + 1;
            const int64_t e0 = triangle_edge(x0, y0, x1, y1, px2, py2);
            const int64_t e1 = triangle_edge(x1, y1, x2, y2, px2, py2);
            const int64_t e2 = triangle_edge(x2, y2, x0, y0, px2, py2);
            const bool reverse = area > 0;
            const bool positive_area = area > 0;
            const bool inside =
                triangle_edge_inside(e0, x0, y0, x1, y1,
                                     reverse, positive_area) &&
                triangle_edge_inside(e1, x1, y1, x2, y2,
                                     reverse, positive_area) &&
                triangle_edge_inside(e2, x2, y2, x0, y0,
                                     reverse, positive_area);
            if (inside)
                gpu->vram[(unsigned)y * PSX_GPU_VRAM_WIDTH +
                          (unsigned)x] = color;
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
    gpu->parameters[2] = 0u;
    gpu->draw_area_left = 0u;
    gpu->draw_area_top = 0u;
    gpu->draw_area_right = PSX_GPU_VRAM_WIDTH - 1u;
    gpu->draw_area_bottom = PSX_GPU_VRAM_HEIGHT - 1u;
    gpu->draw_offset_x = 0;
    gpu->draw_offset_y = 0;
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
            else if (opcode == 0x20u)
                execute_flat_triangle(gpu);
            else if (opcode == 0x30u)
                execute_gouraud_triangle(gpu);
            gpu->parameter_count = 0u;
            gpu->parameter_expected = 0u;
        }
        return true;
    }

    gpu->command = value;
    switch (value >> 24u) {
    case 0xe3u: /* Drawing area top-left. */
        gpu->draw_area_left = value & 0x3ffu;
        gpu->draw_area_top = (value >> 10u) & 0x1ffu;
        break;
    case 0xe4u: /* Drawing area bottom-right. */
        gpu->draw_area_right = value & 0x3ffu;
        gpu->draw_area_bottom = (value >> 10u) & 0x1ffu;
        break;
    case 0xe5u: /* Drawing offset, signed 11-bit coordinates. */
        gpu->draw_offset_x = sign_extend_11(value);
        gpu->draw_offset_y = sign_extend_11(value >> 11u);
        break;
    case 0x20u: /* Flat-shaded triangle */
        gpu->parameter_expected = 3u;
        gpu->parameter_count = 0u;
        break;
    case 0x30u: /* Gouraud-shaded triangle */
        gpu->parameter_expected = 5u;
        gpu->parameter_count = 0u;
        break;
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
