#include "dreampsx/gte.h"

#include <stddef.h>
#include <string.h>

static uint32_t sign_extend_16(uint32_t value)
{
    uint32_t low = value & 0xffffu;
    return (low & 0x8000u) ? (low | 0xffff0000u) : low;
}

static uint32_t leading_sign_bits(uint32_t value)
{
    uint32_t count = 0u;
    bool sign = (value & 0x80000000u) != 0u;
    for (uint32_t bit = 0x40000000u; bit != 0u; bit >>= 1u) {
        if (((value & bit) != 0u) != sign) break;
        ++count;
    }
    return count;
}

static uint32_t clamp_irgb_channel(uint32_t value)
{
    uint32_t channel = value >> 7u;
    if (channel > 31u) channel = 31u;
    return channel;
}

void psx_gte_reset(PsxGte *gte)
{
    if (!gte) return;
    memset(gte, 0, sizeof(*gte));
}

bool psx_gte_read_data(const PsxGte *gte, uint8_t index, uint32_t *value)
{
    uint32_t result;
    if (!gte || !value || index >= PSX_GTE_REGISTER_COUNT) return false;

    switch (index) {
    case 8u:
    case 9u:
    case 10u:
    case 11u:
        result = sign_extend_16(gte->data[index]);
        break;
    case 15u:
        result = gte->data[14u];
        break;
    case 28u:
    case 29u: {
        uint32_t r = clamp_irgb_channel(gte->data[9u]);
        uint32_t g = clamp_irgb_channel(gte->data[10u]);
        uint32_t b = clamp_irgb_channel(gte->data[11u]);
        result = r | (g << 5u) | (b << 10u);
        break;
    }
    case 31u:
        result = gte->data[31u];
        break;
    default:
        result = gte->data[index];
        break;
    }

    *value = result;
    return true;
}

bool psx_gte_write_data(PsxGte *gte, uint8_t index, uint32_t value)
{
    if (!gte || index >= PSX_GTE_REGISTER_COUNT) return false;

    switch (index) {
    case 8u:
    case 9u:
    case 10u:
    case 11u:
        gte->data[index] = sign_extend_16(value);
        break;
    case 15u:
        gte->data[12u] = gte->data[13u];
        gte->data[13u] = gte->data[14u];
        gte->data[14u] = value;
        break;
    case 28u:
        gte->data[9u] = ((value & 0x001fu) << 7u);
        gte->data[10u] = (((value >> 5u) & 0x001fu) << 7u);
        gte->data[11u] = (((value >> 10u) & 0x001fu) << 7u);
        gte->data[28u] = value & 0x7fffu;
        break;
    case 29u:
        /* ORGB is a read-only view of the packed IR channels. */
        break;
    case 30u:
        gte->data[30u] = value;
        gte->data[31u] = leading_sign_bits(value);
        break;
    case 31u:
        /* LZCR is read-only; it is computed from LZCS. */
        break;
    default:
        gte->data[index] = value;
        break;
    }
    return true;
}

bool psx_gte_read_control(const PsxGte *gte, uint8_t index, uint32_t *value)
{
    if (!gte || !value || index >= PSX_GTE_REGISTER_COUNT) return false;
    *value = gte->control[index];
    return true;
}

bool psx_gte_write_control(PsxGte *gte, uint8_t index, uint32_t value)
{
    if (!gte || index >= PSX_GTE_REGISTER_COUNT) return false;
    if (index == 31u) value &= 0x7ffff000u;
    gte->control[index] = value;
    if (index == 31u && (value & 0x7ffff000u) != 0u)
        gte->control[index] |= 0x80000000u;
    return true;
}

static int32_t gte_signed16(uint32_t value)
{
    uint32_t low = value & 0xffffu;
    return (low & 0x8000u) ? (int32_t)(low - 0x10000u) : (int32_t)low;
}

static int32_t gte_matrix_element(const PsxGte *gte, unsigned matrix,
                                    unsigned row, unsigned column)
{
    static const unsigned bases[3] = {0u, 8u, 16u};
    unsigned base = bases[matrix];
    unsigned pair = row * 3u + column;
    uint32_t packed = gte->control[base + pair / 2u];
    return gte_signed16((pair & 1u) ? (packed >> 16u) : packed);
}

static int64_t gte_shift_mac(int64_t value, unsigned shift)
{
    if (shift == 0u) return value;
    if (value >= 0) return value / (INT64_C(1) << shift);
    return -(((-value) + ((INT64_C(1) << shift) - 1)) /
             (INT64_C(1) << shift));
}

static void gte_check_mac_overflow(int64_t value, unsigned component,
                                   uint32_t *flags)
{
    if (value > INT64_C(0x7ffffffffff))
        *flags |= 1u << (30u - component * 2u);
    if (value < -INT64_C(0x80000000000))
        *flags |= 1u << (27u - component * 2u);
}

static int32_t gte_saturate_ir(int64_t value, unsigned component, bool lm,
                               uint32_t *flags)
{
    int64_t minimum = lm ? 0 : -32768;
    if (value > 32767) {
        *flags |= 1u << (24u - component);
        return 32767;
    }
    if (value < minimum) {
        *flags |= 1u << (24u - component);
        return (int32_t)minimum;
    }
    return (int32_t)value;
}

static void gte_execute_mvmva(PsxGte *gte, uint32_t instruction)
{
    unsigned matrix = (instruction >> 17u) & 3u;
    unsigned vector = (instruction >> 15u) & 3u;
    unsigned translation = (instruction >> 13u) & 3u;
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t v[3], m[3][3], t[3];
    uint32_t flags = 0u;
    unsigned i, j;

    for (i = 0u; i < 3u; ++i) {
        if (vector == 3u)
            v[i] = gte_signed16(gte->data[9u + i]);
        else
            v[i] = gte_signed16(gte->data[vector * 2u + (i == 2u ? 1u : 0u)] >>
                                ((i == 1u) ? 16u : 0u));
    }

    for (i = 0u; i < 3u; ++i) {
        for (j = 0u; j < 3u; ++j) {
            if (matrix == 3u) {
                int32_t red = (int32_t)(gte->data[6u] & 0xffu) << 4u;
                if (i == 0u && j == 0u) m[i][j] = -red;
                else if (i == 0u && j == 1u) m[i][j] = red;
                else if (i == 0u && j == 2u) m[i][j] = gte_signed16(gte->data[8u]);
                else if (i == 1u) m[i][j] = gte_matrix_element(gte, 0u, 0u, 2u);
                else m[i][j] = gte_matrix_element(gte, 0u, 1u, 1u);
            } else {
                m[i][j] = gte_matrix_element(gte, matrix, i, j);
            }
        }
    }

    for (i = 0u; i < 3u; ++i) {
        if (translation == 0u) t[i] = (int32_t)gte->control[5u + i];
        else if (translation == 1u) t[i] = (int32_t)gte->control[13u + i];
        else if (translation == 2u) t[i] = (int32_t)gte->control[21u + i];
        else t[i] = 0;
    }

    for (i = 0u; i < 3u; ++i) {
        int64_t accumulator;
        int64_t shifted;
        if (translation == 2u) {
            /* The FC hardware quirk performs an extra first-stage clamp.
             * Its MAC is discarded, but its overflow and IR saturation flags
             * remain observable after the final result overwrites IR. */
            accumulator = (int64_t)t[i] * 4096;
            accumulator += (int64_t)m[i][0] * v[0];
            gte_check_mac_overflow(accumulator, i, &flags);
            (void)gte_saturate_ir(gte_shift_mac(accumulator, shift), i,
                                  lm, &flags);

            accumulator = (int64_t)m[i][1] * v[1];
            gte_check_mac_overflow(accumulator, i, &flags);
            accumulator += (int64_t)m[i][2] * v[2];
            gte_check_mac_overflow(accumulator, i, &flags);
        } else {
            accumulator = (int64_t)t[i] * 4096;
            gte_check_mac_overflow(accumulator, i, &flags);
            accumulator += (int64_t)m[i][0] * v[0];
            gte_check_mac_overflow(accumulator, i, &flags);
            accumulator += (int64_t)m[i][1] * v[1];
            gte_check_mac_overflow(accumulator, i, &flags);
            accumulator += (int64_t)m[i][2] * v[2];
            gte_check_mac_overflow(accumulator, i, &flags);
        }
        shifted = gte_shift_mac(accumulator, shift);
        gte->data[25u + i] = (uint32_t)shifted;
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            shifted, i, lm, &flags);
    }
    if (flags & 0x7f800000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}


static void gte_check_mac0_overflow(int64_t value, uint32_t *flags)
{
    if (value > INT32_MAX) *flags |= 1u << 16u;
    if (value < INT32_MIN) *flags |= 1u << 15u;
}

static int32_t gte_saturate_screen(int64_t value, unsigned flag_bit,
                                   uint32_t *flags)
{
    if (value > 1023) {
        *flags |= 1u << flag_bit;
        return 1023;
    }
    if (value < -1024) {
        *flags |= 1u << flag_bit;
        return -1024;
    }
    return (int32_t)value;
}

static uint32_t gte_unr_divide(uint16_t h, uint16_t sz, uint32_t *flags)
{
    uint32_t z, n, d, u, index, table;
    if ((uint32_t)h >= ((uint32_t)sz * 2u)) {
        *flags |= 1u << 17u;
        return 0x1ffffu;
    }

    /* The PS1 uses a 257-entry unsigned Newton-Raphson reciprocal table.
     * Its entries are generated by the documented integer expression. */
    z = 0u;
    while (z < 16u && ((uint32_t)sz << z) < 0x8000u) ++z;
    n = (uint32_t)h << z;
    d = (uint32_t)sz << z;
    index = (d - 0x7fc0u) >> 7u;
    table = 0x40000u / (index + 0x100u);
    table = (table + 1u) / 2u;
    u = table > 0x101u ? table - 0x101u : 0u;
    u += 0x101u;
    d = (0x2000080u - d * u) >> 8u;
    d = (0x80u + d * u) >> 8u;
    n = (uint32_t)(((uint64_t)n * d + 0x8000u) >> 16u);
    return n > 0x1ffffu ? 0x1ffffu : n;
}

static void gte_push_sxy(PsxGte *gte, uint32_t value)
{
    gte->data[12] = gte->data[13];
    gte->data[13] = gte->data[14];
    gte->data[14] = value;
}

static void gte_push_sz(PsxGte *gte, uint32_t value)
{
    gte->data[16] = gte->data[17];
    gte->data[17] = gte->data[18];
    gte->data[18] = gte->data[19];
    gte->data[19] = value & 0xffffu;
}

static void gte_execute_rtps(PsxGte *gte, uint32_t instruction,
                             unsigned vertex, uint32_t *accumulated_flags)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    int32_t v[3], ir[3];
    int64_t mac[3], shifted[3];
    uint32_t flags = 0u, h, sz, quotient, sxy;
    int64_t mac0;
    unsigned i, j;

    v[0] = gte_signed16(gte->data[vertex * 2u]);
    v[1] = gte_signed16(gte->data[vertex * 2u] >> 16u);
    v[2] = gte_signed16(gte->data[vertex * 2u + 1u]);

    for (i = 0u; i < 3u; ++i) {
        mac[i] = (int64_t)(int32_t)gte->control[5u + i] * 4096;
        gte_check_mac_overflow(mac[i], i, &flags);
        for (j = 0u; j < 3u; ++j) {
            mac[i] += (int64_t)gte_matrix_element(gte, 0u, i, j) * v[j];
            gte_check_mac_overflow(mac[i], i, &flags);
        }
        shifted[i] = gte_shift_mac(mac[i], shift);
        gte->data[25u + i] = (uint32_t)shifted[i];
        if (i == 2u) {
            bool lm = (instruction & (1u << 10u)) != 0u;
            int64_t stored = shifted[i];
            /* FLAG.22 checks MAC3>>12 as if lm=0, independently of sf. */
            (void)gte_saturate_ir(gte_shift_mac(mac[i], 12u), 2u,
                                  false, &flags);
            if (stored > 32767) stored = 32767;
            if (stored < (lm ? 0 : -32768)) stored = lm ? 0 : -32768;
            ir[i] = (int32_t)stored;
        } else {
            ir[i] = gte_saturate_ir(shifted[i], i,
                                    (instruction & (1u << 10u)) != 0u,
                                    &flags);
        }
        gte->data[9u + i] = (uint32_t)ir[i];
    }

    /* SZ3 uses MAC3>>12 when sf=0 and MAC3 directly when sf=1. */
    {
        int64_t depth = gte_shift_mac(mac[2], 12u);
        if (depth < 0) depth = 0;
        if (depth > 0xffff) {
            flags |= 1u << 18u;
            depth = 0xffff;
        }
        sz = (uint32_t)depth;
    }
    gte_push_sz(gte, sz);

    h = gte->control[26] & 0xffffu;
    quotient = gte_unr_divide((uint16_t)h, (uint16_t)sz, &flags);

    mac0 = (int64_t)quotient * ir[0] + (int32_t)gte->control[24];
    gte_check_mac0_overflow(mac0, &flags);
    gte->data[24] = (uint32_t)mac0;
    {
        int32_t sx = gte_saturate_screen(gte_shift_mac(mac0, 16u), 14u, &flags);
        mac0 = (int64_t)quotient * ir[1] + (int32_t)gte->control[25];
        gte_check_mac0_overflow(mac0, &flags);
        gte->data[24] = (uint32_t)mac0;
        {
            int32_t sy = gte_saturate_screen(gte_shift_mac(mac0, 16u), 13u, &flags);
            sxy = (uint16_t)sx | ((uint32_t)(uint16_t)sy << 16u);
        }
    }
    gte_push_sxy(gte, sxy);

    mac0 = (int64_t)(int16_t)gte->control[27] * quotient +
           (int32_t)gte->control[28];
    gte_check_mac0_overflow(mac0, &flags);
    gte->data[24] = (uint32_t)mac0;
    {
        int64_t ir0 = gte_shift_mac(mac0, 12u);
        if (ir0 < 0) {
            flags |= 1u << 12u;
            ir0 = 0;
        } else if (ir0 > 0x1000) {
            flags |= 1u << 12u;
            ir0 = 0x1000;
        }
        gte->data[8] = (uint32_t)ir0;
    }

    *accumulated_flags |= flags;
    if (*accumulated_flags & 0x7f87e000u) *accumulated_flags |= 0x80000000u;
    gte->control[31] = *accumulated_flags;
}

static void gte_execute_rtpt(PsxGte *gte, uint32_t instruction)
{
    uint32_t accumulated_flags = 0u;
    unsigned vertex;
    for (vertex = 0u; vertex < 3u; ++vertex)
        gte_execute_rtps(gte, instruction, vertex, &accumulated_flags);
}

static void gte_push_rgb(PsxGte *gte, uint32_t value)
{
    gte->data[20] = gte->data[21];
    gte->data[21] = gte->data[22];
    gte->data[22] = value;
}

static uint32_t gte_color_channel(int64_t mac)
{
    int64_t channel = gte_shift_mac(mac, 4u);
    if (channel < 0) channel = 0;
    if (channel > 255) channel = 255;
    return (uint32_t)channel;
}

static void gte_execute_interpolate(PsxGte *gte, uint32_t instruction,
                                       bool from_rgb, uint32_t source_rgb,
                                       uint32_t *accumulated_flags)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t ir0 = gte_signed16(gte->data[8]);
    const uint32_t channels[3] = {
        source_rgb & 0xffu,
        (source_rgb >> 8u) & 0xffu,
        (source_rgb >> 16u) & 0xffu
    };
    uint32_t flags = 0u;
    uint32_t rgb = gte->data[6] & 0xff000000u;
    int64_t mac[3];
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        int64_t source = from_rgb
            ? ((int64_t)channels[i] << 16u)
            : ((int64_t)gte_signed16(gte->data[9u + i]) << 12u);
        int64_t far = (int64_t)(int32_t)gte->control[21u + i] * 4096;
        int32_t difference = gte_saturate_ir(
            gte_shift_mac(far - source, shift), i, false, &flags);

        mac[i] = source + (int64_t)ir0 * difference;
        gte_check_mac_overflow(mac[i], i, &flags);
        mac[i] = gte_shift_mac(mac[i], shift);
        gte->data[25u + i] = (uint32_t)mac[i];
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            mac[i], i, lm, &flags);
    }

    rgb |= gte_color_channel(mac[0]) |
           (gte_color_channel(mac[1]) << 8u) |
           (gte_color_channel(mac[2]) << 16u);
    gte_push_rgb(gte, rgb);
    if (accumulated_flags) {
        *accumulated_flags |= flags;
    } else {
        if (flags & 0x7f87e000u) flags |= 0x80000000u;
        gte->control[31] = flags;
    }
}

static void gte_execute_dpct(PsxGte *gte, uint32_t instruction)
{
    uint32_t accumulated_flags = 0u;
    unsigned i;

    /* Each iteration consumes the current oldest RGB FIFO entry. The push
     * shifts the next original color into RGB0 for the following iteration. */
    for (i = 0u; i < 3u; ++i) {
        uint32_t source_rgb = gte->data[20];
        gte_execute_interpolate(gte, instruction, true, source_rgb,
                                &accumulated_flags);
    }

    if (accumulated_flags & 0x7f87e000u)
        accumulated_flags |= 0x80000000u;
    gte->control[31] = accumulated_flags;
}

static void gte_execute_ncs(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t normal[3];
    int32_t light[3];
    int64_t light_mac[3], color_mac[3];
    uint32_t flags = 0u;
    uint32_t code = gte->data[6] & 0xff000000u;
    uint32_t rgb;
    unsigned i, j;

    normal[0] = gte_signed16(gte->data[0]);
    normal[1] = gte_signed16(gte->data[0] >> 16u);
    normal[2] = gte_signed16(gte->data[1]);

    /* Stage 1: normal transformed by the light matrix. */
    for (i = 0u; i < 3u; ++i) {
        light_mac[i] = 0;
        for (j = 0u; j < 3u; ++j) {
            light_mac[i] += (int64_t)gte_matrix_element(gte, 1u, i, j) *
                            normal[j];
            gte_check_mac_overflow(light_mac[i], i, &flags);
        }
        light[i] = gte_saturate_ir(gte_shift_mac(light_mac[i], shift),
                                   i, lm, &flags);
    }

    /* Stage 2: color matrix plus background color. */
    for (i = 0u; i < 3u; ++i) {
        color_mac[i] = (int64_t)(int32_t)gte->control[13u + i] * 4096;
        gte_check_mac_overflow(color_mac[i], i, &flags);
        for (j = 0u; j < 3u; ++j) {
            color_mac[i] += (int64_t)gte_matrix_element(gte, 2u, i, j) *
                            light[j];
            gte_check_mac_overflow(color_mac[i], i, &flags);
        }
        color_mac[i] = gte_shift_mac(color_mac[i], shift);
        gte->data[25u + i] = (uint32_t)color_mac[i];
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            color_mac[i], i, lm, &flags);
    }

    rgb = gte_color_channel(color_mac[0]) |
          (gte_color_channel(color_mac[1]) << 8u) |
          (gte_color_channel(color_mac[2]) << 16u) | code;
    gte_push_rgb(gte, rgb);

    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_nccs(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t normal[3], light[3];
    int64_t light_mac[3], color_mac[3];
    uint32_t flags = 0u, code = gte->data[6] & 0xff000000u;
    uint32_t rgb;
    unsigned i, j;

    normal[0] = gte_signed16(gte->data[0]);
    normal[1] = gte_signed16(gte->data[0] >> 16u);
    normal[2] = gte_signed16(gte->data[1]);

    /* Stage 1: transform the normal by the light matrix. */
    for (i = 0u; i < 3u; ++i) {
        light_mac[i] = 0;
        for (j = 0u; j < 3u; ++j) {
            light_mac[i] += (int64_t)gte_matrix_element(gte, 1u, i, j) *
                            normal[j];
            gte_check_mac_overflow(light_mac[i], i, &flags);
        }
        light[i] = gte_saturate_ir(gte_shift_mac(light_mac[i], shift),
                                   i, lm, &flags);
    }

    /* Stage 2: background plus color-matrix transform. */
    for (i = 0u; i < 3u; ++i) {
        color_mac[i] = (int64_t)(int32_t)gte->control[13u + i] * 4096;
        gte_check_mac_overflow(color_mac[i], i, &flags);
        for (j = 0u; j < 3u; ++j) {
            color_mac[i] += (int64_t)gte_matrix_element(gte, 2u, i, j) *
                            light[j];
            gte_check_mac_overflow(color_mac[i], i, &flags);
        }
        color_mac[i] = gte_shift_mac(color_mac[i], shift);
        light[i] = gte_saturate_ir(color_mac[i], i, lm, &flags);
    }

    /* Stage 3: modulate by RGBC and retain the GTE's extra four-bit scale. */
    {
        const uint32_t channels[3] = {
            gte->data[6] & 0xffu,
            (gte->data[6] >> 8u) & 0xffu,
            (gte->data[6] >> 16u) & 0xffu
        };
        for (i = 0u; i < 3u; ++i) {
            int64_t mac = ((int64_t)channels[i] * light[i]) * 16;
            int64_t result;
            gte_check_mac_overflow(mac, i, &flags);
            result = gte_shift_mac(mac, shift);
            gte->data[25u + i] = (uint32_t)result;
            gte->data[9u + i] = (uint32_t)gte_saturate_ir(
                result, i, lm, &flags);
            color_mac[i] = result;
        }
    }

    rgb = gte_color_channel(color_mac[0]) |
          (gte_color_channel(color_mac[1]) << 8u) |
          (gte_color_channel(color_mac[2]) << 16u) | code;
    gte_push_rgb(gte, rgb);

    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_dcpl(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t normal[3], light[3];
    int64_t mac[3];
    const uint32_t channels[3] = {
        gte->data[6] & 0xffu,
        (gte->data[6] >> 8u) & 0xffu,
        (gte->data[6] >> 16u) & 0xffu
    };
    int32_t ir0 = gte_signed16(gte->data[8]);
    uint32_t flags = 0u;
    uint32_t rgb = gte->data[6] & 0xff000000u;
    unsigned i, j;

    normal[0] = gte_signed16(gte->data[0]);
    normal[1] = gte_signed16(gte->data[0] >> 16u);
    normal[2] = gte_signed16(gte->data[1]);

    /* Light matrix stage. */
    for (i = 0u; i < 3u; ++i) {
        int64_t accumulator = 0;
        for (j = 0u; j < 3u; ++j) {
            accumulator += (int64_t)gte_matrix_element(gte, 1u, i, j) *
                           normal[j];
            gte_check_mac_overflow(accumulator, i, &flags);
        }
        light[i] = gte_saturate_ir(gte_shift_mac(accumulator, shift),
                                   i, lm, &flags);
    }

    /* Color matrix plus background stage. */
    for (i = 0u; i < 3u; ++i) {
        int64_t accumulator = (int64_t)(int32_t)gte->control[13u + i] * 4096;
        gte_check_mac_overflow(accumulator, i, &flags);
        for (j = 0u; j < 3u; ++j) {
            accumulator += (int64_t)gte_matrix_element(gte, 2u, i, j) *
                           light[j];
            gte_check_mac_overflow(accumulator, i, &flags);
        }
        light[i] = gte_saturate_ir(gte_shift_mac(accumulator, shift),
                                   i, lm, &flags);
    }

    /* RGBC modulation followed by far-color interpolation. */
    for (i = 0u; i < 3u; ++i) {
        int64_t base = (int64_t)channels[i] * light[i] * 16;
        int64_t far = (int64_t)(int32_t)gte->control[21u + i] * 4096;
        int32_t difference;
        gte_check_mac_overflow(base, i, &flags);
        difference = gte_saturate_ir(
            gte_shift_mac(far - base, shift), i, false, &flags);
        mac[i] = base + (int64_t)ir0 * difference;
        gte_check_mac_overflow(mac[i], i, &flags);
        mac[i] = gte_shift_mac(mac[i], shift);
        gte->data[25u + i] = (uint32_t)mac[i];
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            mac[i], i, lm, &flags);
    }

    rgb |= gte_color_channel(mac[0]) |
           (gte_color_channel(mac[1]) << 8u) |
           (gte_color_channel(mac[2]) << 16u);
    gte_push_rgb(gte, rgb);
    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_cc(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t input_ir[3], color_ir[3];
    int64_t mac[3];
    const uint32_t channels[3] = {
        gte->data[6] & 0xffu,
        (gte->data[6] >> 8u) & 0xffu,
        (gte->data[6] >> 16u) & 0xffu
    };
    uint32_t flags = 0u;
    uint32_t rgb = gte->data[6] & 0xff000000u;
    unsigned i, j;

    input_ir[0] = gte_signed16(gte->data[9]);
    input_ir[1] = gte_signed16(gte->data[10]);
    input_ir[2] = gte_signed16(gte->data[11]);

    /* Background plus color matrix, followed by RGBC modulation. */
    for (i = 0u; i < 3u; ++i) {
        int64_t accumulator = (int64_t)(int32_t)gte->control[13u + i] * 4096;
        gte_check_mac_overflow(accumulator, i, &flags);
        for (j = 0u; j < 3u; ++j) {
            accumulator += (int64_t)gte_matrix_element(gte, 2u, i, j) *
                           input_ir[j];
            gte_check_mac_overflow(accumulator, i, &flags);
        }
        color_ir[i] = gte_saturate_ir(
            gte_shift_mac(accumulator, shift), i, lm, &flags);
    }

    for (i = 0u; i < 3u; ++i) {
        mac[i] = (int64_t)channels[i] * color_ir[i] * 16;
        gte_check_mac_overflow(mac[i], i, &flags);
        mac[i] = gte_shift_mac(mac[i], shift);
        gte->data[25u + i] = (uint32_t)mac[i];
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            mac[i], i, lm, &flags);
    }

    rgb |= gte_color_channel(mac[0]) |
           (gte_color_channel(mac[1]) << 8u) |
           (gte_color_channel(mac[2]) << 16u);
    gte_push_rgb(gte, rgb);
    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_cdp(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t input_ir[3], color_ir[3];
    int64_t mac[3];
    const uint32_t channels[3] = {
        gte->data[6] & 0xffu,
        (gte->data[6] >> 8u) & 0xffu,
        (gte->data[6] >> 16u) & 0xffu
    };
    int32_t ir0 = gte_signed16(gte->data[8]);
    uint32_t flags = 0u;
    uint32_t rgb = gte->data[6] & 0xff000000u;
    unsigned i, j;

    input_ir[0] = gte_signed16(gte->data[9]);
    input_ir[1] = gte_signed16(gte->data[10]);
    input_ir[2] = gte_signed16(gte->data[11]);

    /* BK + color matrix * IR. */
    for (i = 0u; i < 3u; ++i) {
        int64_t accumulator = (int64_t)(int32_t)gte->control[13u + i] * 4096;
        gte_check_mac_overflow(accumulator, i, &flags);
        for (j = 0u; j < 3u; ++j) {
            accumulator += (int64_t)gte_matrix_element(gte, 2u, i, j) *
                           input_ir[j];
            gte_check_mac_overflow(accumulator, i, &flags);
        }
        accumulator = gte_shift_mac(accumulator, shift);
        color_ir[i] = gte_saturate_ir(accumulator, i, lm, &flags);
    }

    /* RGBC modulation followed by far-color interpolation. */
    for (i = 0u; i < 3u; ++i) {
        int64_t base = (int64_t)channels[i] * color_ir[i] * 16;
        int64_t far = (int64_t)(int32_t)gte->control[21u + i] * 4096;
        int32_t difference;
        gte_check_mac_overflow(base, i, &flags);
        difference = gte_saturate_ir(
            gte_shift_mac(far - base, shift), i, false, &flags);
        mac[i] = base + (int64_t)ir0 * difference;
        gte_check_mac_overflow(mac[i], i, &flags);
        mac[i] = gte_shift_mac(mac[i], shift);
        gte->data[25u + i] = (uint32_t)mac[i];
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            mac[i], i, lm, &flags);
    }

    rgb |= gte_color_channel(mac[0]) |
           (gte_color_channel(mac[1]) << 8u) |
           (gte_color_channel(mac[2]) << 16u);
    gte_push_rgb(gte, rgb);
    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_ncds(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t normal[3], light[3];
    const uint32_t channels[3] = {
        gte->data[6] & 0xffu,
        (gte->data[6] >> 8u) & 0xffu,
        (gte->data[6] >> 16u) & 0xffu
    };
    int32_t ir0 = gte_signed16(gte->data[8]);
    int64_t mac[3];
    uint32_t flags = 0u;
    uint32_t rgb = gte->data[6] & 0xff000000u;
    unsigned i, j;

    normal[0] = gte_signed16(gte->data[0]);
    normal[1] = gte_signed16(gte->data[0] >> 16u);
    normal[2] = gte_signed16(gte->data[1]);

    /* Stage 1: light matrix times normal. */
    for (i = 0u; i < 3u; ++i) {
        int64_t accumulator = 0;
        for (j = 0u; j < 3u; ++j) {
            accumulator += (int64_t)gte_matrix_element(gte, 1u, i, j) *
                           normal[j];
            gte_check_mac_overflow(accumulator, i, &flags);
        }
        light[i] = gte_saturate_ir(gte_shift_mac(accumulator, shift),
                                   i, lm, &flags);
    }

    /* Stage 2: background plus color matrix. */
    for (i = 0u; i < 3u; ++i) {
        int64_t accumulator = (int64_t)(int32_t)gte->control[13u + i] * 4096;
        gte_check_mac_overflow(accumulator, i, &flags);
        for (j = 0u; j < 3u; ++j) {
            accumulator += (int64_t)gte_matrix_element(gte, 2u, i, j) *
                           light[j];
            gte_check_mac_overflow(accumulator, i, &flags);
        }
        accumulator = gte_shift_mac(accumulator, shift);
        light[i] = gte_saturate_ir(accumulator, i, lm, &flags);
    }

    /* Stage 3: modulate the color, then interpolate toward far color. */
    for (i = 0u; i < 3u; ++i) {
        int64_t base = (int64_t)channels[i] * light[i] * 16;
        int64_t far = (int64_t)(int32_t)gte->control[21u + i] * 4096;
        int32_t difference;
        gte_check_mac_overflow(base, i, &flags);
        difference = gte_saturate_ir(
            gte_shift_mac(far - base, shift), i, false, &flags);
        mac[i] = base + (int64_t)ir0 * difference;
        gte_check_mac_overflow(mac[i], i, &flags);
        mac[i] = gte_shift_mac(mac[i], shift);
        gte->data[25u + i] = (uint32_t)mac[i];
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            mac[i], i, lm, &flags);
    }

    rgb |= gte_color_channel(mac[0]) |
           (gte_color_channel(mac[1]) << 8u) |
           (gte_color_channel(mac[2]) << 16u);
    gte_push_rgb(gte, rgb);
    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_ncdt(PsxGte *gte, uint32_t instruction)
{
    uint32_t saved_v0 = gte->data[0];
    uint32_t saved_v0z = gte->data[1];
    uint32_t accumulated_flags = 0u;
    unsigned vertex;

    for (vertex = 0u; vertex < 3u; ++vertex) {
        uint32_t flags;
        gte->data[0] = gte->data[vertex * 2u];
        gte->data[1] = gte->data[vertex * 2u + 1u];
        gte_execute_ncds(gte, instruction);
        flags = gte->control[31];
        accumulated_flags |= flags & 0x7fffffffu;
    }

    gte->data[0] = saved_v0;
    gte->data[1] = saved_v0z;
    if (accumulated_flags & 0x7f87e000u)
        accumulated_flags |= 0x80000000u;
    gte->control[31] = accumulated_flags;
}

static void gte_execute_nct(PsxGte *gte, uint32_t instruction)
{
    uint32_t saved_v0 = gte->data[0];
    uint32_t saved_v0z = gte->data[1];
    uint32_t accumulated_flags = 0u;
    unsigned vertex;

    /* NCT applies the NCS lighting/color pipeline to V0, V1 and V2. */
    for (vertex = 0u; vertex < 3u; ++vertex) {
        uint32_t flags;
        gte->data[0] = gte->data[vertex * 2u];
        gte->data[1] = gte->data[vertex * 2u + 1u];
        gte_execute_ncs(gte, instruction);
        flags = gte->control[31];
        accumulated_flags |= flags & 0x7fffffffu;
    }

    gte->data[0] = saved_v0;
    gte->data[1] = saved_v0z;
    if (accumulated_flags & 0x7f87e000u)
        accumulated_flags |= 0x80000000u;
    gte->control[31] = accumulated_flags;
}

static void gte_execute_ncct(PsxGte *gte, uint32_t instruction)
{
    uint32_t saved_v0 = gte->data[0];
    uint32_t saved_v0z = gte->data[1];
    uint32_t accumulated_flags = 0u;
    unsigned vertex;

    /* NCCT applies the complete NCCS pipeline to V0, V1 and V2. */
    for (vertex = 0u; vertex < 3u; ++vertex) {
        uint32_t flags;
        gte->data[0] = gte->data[vertex * 2u];
        gte->data[1] = gte->data[vertex * 2u + 1u];
        gte_execute_nccs(gte, instruction);
        flags = gte->control[31];
        accumulated_flags |= flags & 0x7fffffffu;
    }

    gte->data[0] = saved_v0;
    gte->data[1] = saved_v0z;
    if (accumulated_flags & 0x7f87e000u)
        accumulated_flags |= 0x80000000u;
    gte->control[31] = accumulated_flags;
}

static void gte_execute_gpl(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t ir0 = gte_signed16(gte->data[8]);
    uint32_t flags = 0u;
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        int32_t ir = gte_signed16(gte->data[9u + i]);
        int32_t previous_mac = (int32_t)gte->data[25u + i];
        int64_t mac = ((int64_t)previous_mac * (INT64_C(1) << shift)) +
                      ((int64_t)ir0 * ir);
        int64_t result;

        gte_check_mac_overflow(mac, i, &flags);
        result = gte_shift_mac(mac, shift);
        gte->data[25u + i] = (uint32_t)result;
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            result, i, lm, &flags);
    }

    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_gpf(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t ir0 = gte_signed16(gte->data[8]);
    uint32_t flags = 0u;
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        int32_t ir = gte_signed16(gte->data[9u + i]);
        int64_t mac = (int64_t)ir0 * ir;
        int64_t result;

        gte_check_mac_overflow(mac, i, &flags);
        result = gte_shift_mac(mac, shift);
        gte->data[25u + i] = (uint32_t)result;
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            result, i, lm, &flags);
    }

    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_op(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    int32_t ir1 = gte_signed16(gte->data[9]);
    int32_t ir2 = gte_signed16(gte->data[10]);
    int32_t ir3 = gte_signed16(gte->data[11]);
    int32_t d1 = gte_matrix_element(gte, 0u, 0u, 0u);
    int32_t d2 = gte_matrix_element(gte, 0u, 1u, 1u);
    int32_t d3 = gte_matrix_element(gte, 0u, 2u, 2u);
    int64_t mac[3];
    uint32_t flags = 0u;
    unsigned i;

    mac[0] = (int64_t)ir3 * d2 - (int64_t)ir2 * d3;
    mac[1] = (int64_t)ir1 * d3 - (int64_t)ir3 * d1;
    mac[2] = (int64_t)ir2 * d1 - (int64_t)ir1 * d2;

    for (i = 0u; i < 3u; ++i) {
        int64_t result;
        gte_check_mac_overflow(mac[i], i, &flags);
        result = gte_shift_mac(mac[i], shift);
        gte->data[25u + i] = (uint32_t)result;
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            result, i, lm, &flags);
    }

    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_sqr(PsxGte *gte, uint32_t instruction)
{
    unsigned shift = (instruction & (1u << 19u)) ? 12u : 0u;
    bool lm = (instruction & (1u << 10u)) != 0u;
    uint32_t flags = 0u;
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        int32_t ir = gte_signed16(gte->data[9u + i]);
        int64_t mac = (int64_t)ir * ir;
        int64_t result;

        gte_check_mac_overflow(mac, i, &flags);
        result = gte_shift_mac(mac, shift);
        gte->data[25u + i] = (uint32_t)result;
        gte->data[9u + i] = (uint32_t)gte_saturate_ir(
            result, i, lm, &flags);
    }

    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

static void gte_execute_avsz(PsxGte *gte, bool four)
{
    unsigned first = four ? 16u : 17u;
    unsigned count = four ? 4u : 3u;
    int64_t sum = 0;
    int64_t mac0;
    int64_t otz;
    uint32_t flags = 0u;
    unsigned i;

    for (i = 0u; i < count; ++i)
        sum += (int64_t)(gte->data[first + i] & 0xffffu);

    mac0 = sum * (int16_t)gte->control[four ? 30u : 29u];
    gte_check_mac0_overflow(mac0, &flags);
    gte->data[24] = (uint32_t)mac0;

    otz = gte_shift_mac(mac0, 12u);
    if (otz < 0) {
        flags |= 1u << 18u;
        otz = 0;
    } else if (otz > 0xffff) {
        flags |= 1u << 18u;
        otz = 0xffff;
    }
    gte->data[7] = (uint32_t)otz;

    if (flags & 0x7f87e000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

bool psx_gte_execute(PsxGte *gte, uint32_t instruction)
{
    uint32_t command;
    int64_t x0, y0, x1, y1, x2, y2, area;
    uint32_t flags = 0u;

    if (!gte) return false;
    command = instruction & 0x3fu;
    if (command == 0x01u) {
        uint32_t accumulated_flags = 0u;
        gte_execute_rtps(gte, instruction, 0u, &accumulated_flags);
        return true;
    }
    if (command == 0x30u) {
        gte_execute_rtpt(gte, instruction);
        return true;
    }
    if (command == 0x3du) {
        gte_execute_gpf(gte, instruction);
        return true;
    }
    if (command == 0x3eu) {
        gte_execute_gpl(gte, instruction);
        return true;
    }
    if (command == 0x10u) {
        gte_execute_interpolate(gte, instruction, true, gte->data[6], NULL);
        return true;
    }
    if (command == 0x11u) {
        gte_execute_interpolate(gte, instruction, false, 0u, NULL);
        return true;
    }
    if (command == 0x2au) {
        gte_execute_dpct(gte, instruction);
        return true;
    }
    if (command == 0x29u) {
        gte_execute_dcpl(gte, instruction);
        return true;
    }
    if (command == 0x14u) {
        gte_execute_cdp(gte, instruction);
        return true;
    }
    if (command == 0x1cu) {
        gte_execute_cc(gte, instruction);
        return true;
    }
    if (command == 0x13u) {
        gte_execute_ncds(gte, instruction);
        return true;
    }
    if (command == 0x16u) {
        gte_execute_ncdt(gte, instruction);
        return true;
    }
    if (command == 0x1eu) {
        gte_execute_ncs(gte, instruction);
        return true;
    }
    if (command == 0x1bu) {
        gte_execute_nccs(gte, instruction);
        return true;
    }
    if (command == 0x20u) {
        gte_execute_nct(gte, instruction);
        return true;
    }
    if (command == 0x3fu) {
        gte_execute_ncct(gte, instruction);
        return true;
    }
    if (command == 0x0cu) {
        gte_execute_op(gte, instruction);
        return true;
    }
    if (command == 0x28u) {
        gte_execute_sqr(gte, instruction);
        return true;
    }
    if (command == 0x2du) {
        gte_execute_avsz(gte, false);
        return true;
    }
    if (command == 0x2eu) {
        gte_execute_avsz(gte, true);
        return true;
    }
    if (command == 0x12u) {
        gte_execute_mvmva(gte, instruction);
        return true;
    }
    if (command != 0x06u) return false; /* NCLIP */

    /* SXY registers pack signed SX in bits 0..15 and SY in bits 16..31. */
    x0 = gte_signed16(gte->data[12]);
    y0 = gte_signed16(gte->data[12] >> 16);
    x1 = gte_signed16(gte->data[13]);
    y1 = gte_signed16(gte->data[13] >> 16);
    x2 = gte_signed16(gte->data[14]);
    y2 = gte_signed16(gte->data[14] >> 16);

    /* Signed screen-space triangle area. Keep the intermediate wide so
     * overflow is detected before MAC0 is reduced to its 32-bit register. */
    area = x0 * (y1 - y2) + x1 * (y2 - y0) + x2 * (y0 - y1);
    gte->data[24] = (uint32_t)area;

    if (area > INT32_MAX) flags |= 1u << 16;
    if (area < INT32_MIN) flags |= 1u << 15;
    if (flags != 0u) flags |= 1u << 31;
    gte->control[31] = flags;
    return true;
}
