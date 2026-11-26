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
    case 28u: {
        uint32_t r = clamp_irgb_channel(gte->data[9u]);
        uint32_t g = clamp_irgb_channel(gte->data[10u]);
        uint32_t b = clamp_irgb_channel(gte->data[11u]);
        result = r | (g << 5u) | (b << 10u);
        break;
    }
    case 29u:
        result = gte->data[28u];
        break;
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
    return true;
}
