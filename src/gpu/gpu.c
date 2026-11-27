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

static void execute_rectangle(PsxGpu *gpu, unsigned width,
                              unsigned height)
{
    const uint32_t xy = gpu->parameters[0];
    const int x = sign_extend_11(xy) + gpu->draw_offset_x;
    const int y = sign_extend_11(xy >> 16u) + gpu->draw_offset_y;
    const int64_t right = (int64_t)x + (int64_t)width - 1;
    const int64_t bottom = (int64_t)y + (int64_t)height - 1;
    const int start_x = x < (int)gpu->draw_area_left ?
                        (int)gpu->draw_area_left : x;
    const int start_y = y < (int)gpu->draw_area_top ?
                        (int)gpu->draw_area_top : y;
    const int end_x = right > (int64_t)gpu->draw_area_right ?
                      (int)gpu->draw_area_right : (int)right;
    const int end_y = bottom > (int64_t)gpu->draw_area_bottom ?
                      (int)gpu->draw_area_bottom : (int)bottom;
    const uint16_t color = rgb24_to_rgb15(gpu->command);

    if (width == 0u || height == 0u || end_x < start_x || end_y < start_y)
        return;

    for (int py = start_y; py <= end_y; ++py) {
        for (int px = start_x; px <= end_x; ++px) {
            gpu->vram[(unsigned)py * PSX_GPU_VRAM_WIDTH + (unsigned)px] =
                color;
        }
    }
}

static void execute_variable_rectangle(PsxGpu *gpu)
{
    const uint32_t wh = gpu->parameters[1];
    execute_rectangle(gpu, wh & 0x3ffu, (wh >> 16u) & 0x1ffu);
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

static unsigned interpolate_uv(unsigned uv0, unsigned uv1, unsigned uv2,
                                    int64_t e0, int64_t e1, int64_t e2,
                                    int64_t area, unsigned shift)
{
    const unsigned c0 = (uv0 >> shift) & 0xffu;
    const unsigned c1 = (uv1 >> shift) & 0xffu;
    const unsigned c2 = (uv2 >> shift) & 0xffu;
    int64_t value = ((int64_t)c0 * e1 + (int64_t)c1 * e2 +
                     (int64_t)c2 * e0) / area;
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    return (unsigned)value;
}

static unsigned apply_texture_window(unsigned coordinate, unsigned mask,
                                     unsigned offset)
{
    const unsigned window_mask = (mask & 0x1fu) << 3u;
    const unsigned window_offset = (offset & mask & 0x1fu) << 3u;
    return (coordinate & ~window_mask) | window_offset;
}

static uint16_t sample_texture(PsxGpu *gpu, unsigned page_x,
                                  unsigned page_y, unsigned depth,
                                  unsigned u, unsigned v, unsigned clut)
{
    unsigned packed_x;
    unsigned shift;
    uint16_t packed;
    unsigned index;
    unsigned clut_x;
    unsigned clut_y;

    if (depth == 2u) {
        return gpu->vram[((page_y + v) % PSX_GPU_VRAM_HEIGHT) *
                         PSX_GPU_VRAM_WIDTH +
                         ((page_x + u) % PSX_GPU_VRAM_WIDTH)];
    }

    if (depth == 0u) {
        packed_x = page_x + (u >> 2u);
        shift = (u & 3u) * 4u;
    } else if (depth == 1u) {
        packed_x = page_x + (u >> 1u);
        shift = (u & 1u) * 8u;
    } else {
        return 0u;
    }

    packed = gpu->vram[((page_y + v) % PSX_GPU_VRAM_HEIGHT) *
                       PSX_GPU_VRAM_WIDTH +
                       (packed_x % PSX_GPU_VRAM_WIDTH)];
    index = (packed >> shift) & (depth == 0u ? 0x0fu : 0xffu);
    clut_x = (clut & 0x3fu) * 16u;
    clut_y = (clut >> 6u) & 0x1ffu;
    return gpu->vram[(clut_y * PSX_GPU_VRAM_WIDTH +
                      ((clut_x + index) % PSX_GPU_VRAM_WIDTH))];
}

static uint16_t blend_semitransparent(uint16_t background,
                                         uint16_t foreground,
                                         unsigned mode)
{
    uint16_t result = 0u;
    for (unsigned shift = 0u; shift <= 10u; shift += 5u) {
        const int back = (background >> shift) & 0x1fu;
        const int front = (foreground >> shift) & 0x1fu;
        int channel;
        switch (mode & 3u) {
        case 0u: channel = (back + front) / 2; break;
        case 1u: channel = back + front; break;
        case 2u: channel = back - front; break;
        default: channel = back + front / 4; break;
        }
        if (channel < 0) channel = 0;
        if (channel > 31) channel = 31;
        result |= (uint16_t)((unsigned)channel << shift);
    }
    return result;
}

static void execute_textured_triangle(PsxGpu *gpu, bool modulate,
                                      bool semi_transparent, bool gouraud)
{
    const uint32_t xy0 = gpu->parameters[0];
    const uint32_t uv0 = gpu->parameters[1];
    const uint32_t xy1 = gpu->parameters[gouraud ? 3u : 2u];
    const uint32_t uv1 = gpu->parameters[gouraud ? 4u : 3u];
    const uint32_t xy2 = gpu->parameters[gouraud ? 6u : 4u];
    const uint32_t uv2 = gpu->parameters[gouraud ? 7u : 5u];
    const uint32_t color1 = gouraud ? gpu->parameters[2] : gpu->command;
    const uint32_t color2 = gouraud ? gpu->parameters[5] : gpu->command;
    const unsigned tpage = uv1 >> 16u;
    const unsigned depth = (tpage >> 7u) & 3u;
    const unsigned page_x = (tpage & 0x0fu) * 64u;
    const unsigned page_y = ((tpage >> 4u) & 1u) * 256u;
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

    const unsigned clut = uv0 >> 16u;

    /* Support 4-bit and 8-bit indexed pages through a VRAM CLUT. */
    if (depth == 3u || area == 0 ||
        max_x < (int)gpu->draw_area_left ||
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
                const unsigned u = interpolate_uv(uv0, uv1, uv2,
                                                   e0, e1, e2, area, 0u);
                const unsigned v = interpolate_uv(uv0, uv1, uv2,
                                                   e0, e1, e2, area, 8u);
                const unsigned window_u = apply_texture_window(
                    u, gpu->texture_window_mask_x,
                    gpu->texture_window_offset_x);
                const unsigned window_v = apply_texture_window(
                    v, gpu->texture_window_mask_y,
                    gpu->texture_window_offset_y);
                const uint16_t texel = sample_texture(
                    gpu, page_x, page_y, depth, window_u, window_v, clut);
                if (texel != 0u) {
                    uint16_t output = texel;
                    if (modulate) {
                        const unsigned vertex_red = gouraud ?
                            interpolate_color_channel(
                                gpu->command & 0xffu, color1 & 0xffu,
                                color2 & 0xffu, e0, e1, e2, area) :
                            (gpu->command & 0xffu);
                        const unsigned vertex_green = gouraud ?
                            interpolate_color_channel(
                                (gpu->command >> 8u) & 0xffu,
                                (color1 >> 8u) & 0xffu,
                                (color2 >> 8u) & 0xffu,
                                e0, e1, e2, area) :
                            ((gpu->command >> 8u) & 0xffu);
                        const unsigned vertex_blue = gouraud ?
                            interpolate_color_channel(
                                (gpu->command >> 16u) & 0xffu,
                                (color1 >> 16u) & 0xffu,
                                (color2 >> 16u) & 0xffu,
                                e0, e1, e2, area) :
                            ((gpu->command >> 16u) & 0xffu);
                        const unsigned red = ((texel & 0x1fu) *
                                              vertex_red) >> 7u;
                        const unsigned green = (((texel >> 5u) & 0x1fu) *
                                                vertex_green) >> 7u;
                        const unsigned blue = (((texel >> 10u) & 0x1fu) *
                                               vertex_blue) >> 7u;
                        const unsigned r = red > 31u ? 31u : red;
                        const unsigned g = green > 31u ? 31u : green;
                        const unsigned b = blue > 31u ? 31u : blue;
                        output = (uint16_t)(r | (g << 5u) | (b << 10u) |
                                            (texel & 0x8000u));
                    }
                    if (semi_transparent && (texel & 0x8000u) != 0u) {
                        const size_t destination =
                            (size_t)(unsigned)y * PSX_GPU_VRAM_WIDTH +
                            (unsigned)x;
                        output = blend_semitransparent(
                            gpu->vram[destination], output,
                            (tpage >> 5u) & 3u);
                    }
                    gpu->vram[(unsigned)y * PSX_GPU_VRAM_WIDTH +
                              (unsigned)x] = output;
                }
            }
        }
    }
}

static void execute_textured_rectangle(PsxGpu *gpu, unsigned width,
                                         unsigned height, bool modulate,
                                         bool semi_transparent)
{
    const uint32_t xy = gpu->parameters[0];
    const uint32_t uv = gpu->parameters[1];
    const unsigned clut = uv >> 16u;
    const unsigned depth = (gpu->draw_mode >> 7u) & 3u;
    const unsigned page_x = (gpu->draw_mode & 0x0fu) * 64u;
    const unsigned page_y = ((gpu->draw_mode >> 4u) & 1u) * 256u;
    const int x = sign_extend_11(xy) + gpu->draw_offset_x;
    const int y = sign_extend_11(xy >> 16u) + gpu->draw_offset_y;
    const int64_t right = (int64_t)x + (int64_t)width - 1;
    const int64_t bottom = (int64_t)y + (int64_t)height - 1;
    const int start_x = x < (int)gpu->draw_area_left ?
                        (int)gpu->draw_area_left : x;
    const int start_y = y < (int)gpu->draw_area_top ?
                        (int)gpu->draw_area_top : y;
    const int end_x = right > (int64_t)gpu->draw_area_right ?
                      (int)gpu->draw_area_right : (int)right;
    const int end_y = bottom > (int64_t)gpu->draw_area_bottom ?
                      (int)gpu->draw_area_bottom : (int)bottom;
    const unsigned base_u = uv & 0xffu;
    const unsigned base_v = (uv >> 8u) & 0xffu;

    if (depth == 3u || width == 0u || height == 0u ||
        end_x < start_x || end_y < start_y)
        return;

    for (int py = start_y; py <= end_y; ++py) {
        for (int px = start_x; px <= end_x; ++px) {
            const unsigned u = (base_u + (unsigned)(px - x)) & 0xffu;
            const unsigned v = (base_v + (unsigned)(py - y)) & 0xffu;
            const unsigned window_u = apply_texture_window(
                u, gpu->texture_window_mask_x,
                gpu->texture_window_offset_x);
            const unsigned window_v = apply_texture_window(
                v, gpu->texture_window_mask_y,
                gpu->texture_window_offset_y);
            const uint16_t texel = sample_texture(
                gpu, page_x, page_y, depth, window_u, window_v, clut);
            if (texel != 0u) {
                uint16_t output = texel;
                if (modulate) {
                    const unsigned red = ((texel & 0x1fu) *
                                          (gpu->command & 0xffu)) >> 7u;
                    const unsigned green = (((texel >> 5u) & 0x1fu) *
                                            ((gpu->command >> 8u) & 0xffu)) >> 7u;
                    const unsigned blue = (((texel >> 10u) & 0x1fu) *
                                           ((gpu->command >> 16u) & 0xffu)) >> 7u;
                    const unsigned r = red > 31u ? 31u : red;
                    const unsigned g = green > 31u ? 31u : green;
                    const unsigned b = blue > 31u ? 31u : blue;
                    output = (uint16_t)(r | (g << 5u) | (b << 10u) |
                                        (texel & 0x8000u));
                }
                if (semi_transparent && (texel & 0x8000u) != 0u) {
                    const size_t destination =
                        (size_t)(unsigned)py * PSX_GPU_VRAM_WIDTH +
                        (unsigned)px;
                    output = blend_semitransparent(
                        gpu->vram[destination], output,
                        (gpu->draw_mode >> 5u) & 3u);
                }
                gpu->vram[(unsigned)py * PSX_GPU_VRAM_WIDTH +
                          (unsigned)px] = output;
            }
        }
    }
}

static void execute_variable_textured_rectangle(PsxGpu *gpu, bool modulate,
                                                bool semi_transparent)
{
    const uint32_t wh = gpu->parameters[2];
    execute_textured_rectangle(gpu, wh & 0x3ffu,
                               (wh >> 16u) & 0x1ffu,
                               modulate, semi_transparent);
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
    gpu->draw_mode = 0u;
    gpu->texture_window_mask_x = 0u;
    gpu->texture_window_mask_y = 0u;
    gpu->texture_window_offset_x = 0u;
    gpu->texture_window_offset_y = 0u;
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
            else if (opcode == 0x60u)
                execute_variable_rectangle(gpu);
            else if (opcode == 0x68u)
                execute_rectangle(gpu, 1u, 1u);
            else if (opcode == 0x70u)
                execute_rectangle(gpu, 8u, 8u);
            else if (opcode == 0x78u)
                execute_rectangle(gpu, 16u, 16u);
            else if (opcode >= 0x64u && opcode <= 0x67u)
                execute_variable_textured_rectangle(
                    gpu, (opcode & 1u) == 0u, (opcode & 2u) != 0u);
            else if (opcode >= 0x74u && opcode <= 0x77u)
                execute_textured_rectangle(
                    gpu, 8u, 8u, (opcode & 1u) == 0u,
                    (opcode & 2u) != 0u);
            else if (opcode >= 0x7cu && opcode <= 0x7fu)
                execute_textured_rectangle(
                    gpu, 16u, 16u, (opcode & 1u) == 0u,
                    (opcode & 2u) != 0u);
            else if (opcode == 0xa0u)
                begin_image_transfer(gpu, PSX_GPU_TRANSFER_CPU_TO_VRAM);
            else if (opcode == 0xc0u)
                begin_image_transfer(gpu, PSX_GPU_TRANSFER_VRAM_TO_CPU);
            else if (opcode == 0x20u)
                execute_flat_triangle(gpu);
            else if (opcode == 0x30u)
                execute_gouraud_triangle(gpu);
            else if (opcode == 0x24u)
                execute_textured_triangle(gpu, true, false, false);
            else if (opcode == 0x25u)
                execute_textured_triangle(gpu, false, false, false);
            else if (opcode == 0x26u)
                execute_textured_triangle(gpu, true, true, false);
            else if (opcode == 0x27u)
                execute_textured_triangle(gpu, false, true, false);
            else if (opcode == 0x34u)
                execute_textured_triangle(gpu, true, false, true);
            else if (opcode == 0x35u)
                execute_textured_triangle(gpu, false, false, true);
            else if (opcode == 0x36u)
                execute_textured_triangle(gpu, true, true, true);
            else if (opcode == 0x37u)
                execute_textured_triangle(gpu, false, true, true);
            gpu->parameter_count = 0u;
            gpu->parameter_expected = 0u;
        }
        return true;
    }

    gpu->command = value;
    switch (value >> 24u) {
    case 0xe1u: /* Draw mode / texture page state. */
        gpu->draw_mode = value & 0x7ffu;
        break;
    case 0xe2u: /* Texture window mask and offset. */
        gpu->texture_window_mask_x = value & 0x1fu;
        gpu->texture_window_mask_y = (value >> 5u) & 0x1fu;
        gpu->texture_window_offset_x = (value >> 10u) & 0x1fu;
        gpu->texture_window_offset_y = (value >> 15u) & 0x1fu;
        break;
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
    case 0x64u: /* Variable-size textured rectangle family */
    case 0x65u:
    case 0x66u:
    case 0x67u:
        gpu->parameter_expected = 3u;
        gpu->parameter_count = 0u;
        break;
    case 0x74u: /* Fixed-size 8x8 textured rectangle family */
    case 0x75u:
    case 0x76u:
    case 0x77u:
    case 0x7cu: /* Fixed-size 16x16 textured rectangle family */
    case 0x7du:
    case 0x7eu:
    case 0x7fu:
        gpu->parameter_expected = 2u;
        gpu->parameter_count = 0u;
        break;
    case 0x60u: /* Variable-size monochrome rectangle */
        gpu->parameter_expected = 2u;
        gpu->parameter_count = 0u;
        break;
    case 0x68u: /* 1x1 monochrome rectangle */
        gpu->parameter_expected = 1u;
        gpu->parameter_count = 0u;
        break;
    case 0x70u: /* 8x8 monochrome rectangle */
    case 0x78u: /* 16x16 monochrome rectangle */
        gpu->parameter_expected = 1u;
        gpu->parameter_count = 0u;
        break;
    case 0x20u: /* Flat-shaded triangle */
        gpu->parameter_expected = 3u;
        gpu->parameter_count = 0u;
        break;
    case 0x30u: /* Gouraud-shaded triangle */
        gpu->parameter_expected = 5u;
        gpu->parameter_count = 0u;
        break;
    case 0x24u: /* Modulated textured triangle */
    case 0x25u: /* Raw textured triangle */
    case 0x26u: /* Modulated semi-transparent textured triangle */
    case 0x27u: /* Raw semi-transparent textured triangle */
        gpu->parameter_expected = 6u;
        gpu->parameter_count = 0u;
        break;
    case 0x34u: /* Gouraud-shaded modulated textured triangle */
    case 0x35u: /* Gouraud-shaded raw textured triangle */
    case 0x36u: /* Gouraud-shaded modulated semi-transparent textured triangle */
    case 0x37u: /* Gouraud-shaded raw semi-transparent textured triangle */
        gpu->parameter_expected = 8u;
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


bool psx_gpu_copy_rgb565(const PsxGpu *gpu, unsigned x, unsigned y,
                         unsigned width, unsigned height,
                         uint16_t *dst, size_t dst_stride)
{
    if (!gpu || !gpu->initialized || !gpu->vram || !dst ||
        width == 0u || height == 0u || dst_stride < width ||
        x >= PSX_GPU_VRAM_WIDTH || y >= PSX_GPU_VRAM_HEIGHT ||
        width > PSX_GPU_VRAM_WIDTH - x ||
        height > PSX_GPU_VRAM_HEIGHT - y)
        return false;

    for (unsigned row = 0u; row < height; ++row) {
        const uint16_t *source = gpu->vram +
            (size_t)(y + row) * PSX_GPU_VRAM_WIDTH + x;
        uint16_t *destination = dst + (size_t)row * dst_stride;
        for (unsigned column = 0u; column < width; ++column) {
            const uint16_t pixel = source[column];
            const unsigned red = pixel & 0x1fu;
            const unsigned green = (pixel >> 5u) & 0x1fu;
            const unsigned blue = (pixel >> 10u) & 0x1fu;
            const unsigned green6 = (green << 1u) | (green >> 4u);
            destination[column] = (uint16_t)(red | (green6 << 5u) |
                                              (blue << 11u));
        }
    }
    return true;
}
