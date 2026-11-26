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
            /* Hardware FC translation bug: the first product contributes to
             * overflow flags but is discarded from the final MAC result. */
            int64_t discarded = (int64_t)t[i] * 4096 + (int64_t)m[i][0] * v[0];
            if (discarded > INT64_C(0x7ffffffffff) ||
                discarded < -INT64_C(0x80000000000))
                flags |= 1u << (30u - i * 2u);
            accumulator = (int64_t)m[i][1] * v[1] +
                          (int64_t)m[i][2] * v[2];
        } else {
            accumulator = (int64_t)t[i] * 4096 +
                          (int64_t)m[i][0] * v[0] +
                          (int64_t)m[i][1] * v[1] +
                          (int64_t)m[i][2] * v[2];
        }
        if (accumulator > INT64_C(0x7ffffffffff))
            flags |= 1u << (30u - i * 2u);
        if (accumulator < -INT64_C(0x80000000000))
            flags |= 1u << (27u - i * 2u);
        shifted = gte_shift_mac(accumulator, shift);
        gte->data[25u + i] = (uint32_t)shifted;
        if (shifted > 32767) {
            flags |= 1u << (24u - i);
            gte->data[9u + i] = lm ? 32767u : 32767u;
        } else if (shifted < (lm ? 0 : -32768)) {
            flags |= 1u << (24u - i);
            gte->data[9u + i] = lm ? 0u : 0xffff8000u;
        } else {
            gte->data[9u + i] = (uint32_t)(int32_t)shifted;
        }
    }
    if (flags & 0x7f800000u) flags |= 0x80000000u;
    gte->control[31] = flags;
}

bool psx_gte_execute(PsxGte *gte, uint32_t instruction)
{
    uint32_t command;
    int64_t x0, y0, x1, y1, x2, y2, area;
    uint32_t flags = 0u;

    if (!gte) return false;
    command = instruction & 0x3fu;
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
