#include "dreampsx/gte.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    PsxGte gte;
    uint32_t value = 0xffffffffu;

    psx_gte_reset(&gte);
    for (uint8_t i = 0u; i < PSX_GTE_REGISTER_COUNT; ++i) {
        assert(psx_gte_read_data(&gte, i, &value));
        assert(value == 0u);
        assert(psx_gte_read_control(&gte, i, &value));
        assert(value == 0u);
    }

    assert(psx_gte_write_data(&gte, 9u, 0x00008001u));
    assert(psx_gte_read_data(&gte, 9u, &value));
    assert(value == 0xffff8001u);

    assert(psx_gte_write_data(&gte, 9u, 0x0800u));
    assert(psx_gte_write_data(&gte, 10u, 0x0300u));
    assert(psx_gte_write_data(&gte, 11u, 0x1f80u));
    assert(psx_gte_read_data(&gte, 28u, &value));
    assert(value == ((16u) | (6u << 5u) | (31u << 10u)));

    assert(psx_gte_write_data(&gte, 28u, 0x00007fffu));
    assert(psx_gte_read_data(&gte, 9u, &value));
    assert(value == (31u << 7u));
    assert(psx_gte_read_data(&gte, 10u, &value));
    assert(value == (31u << 7u));
    assert(psx_gte_read_data(&gte, 11u, &value));
    assert(value == (31u << 7u));
    assert(psx_gte_read_data(&gte, 29u, &value));
    assert(value == 0x7fffu);

    assert(psx_gte_write_data(&gte, 30u, 0x00ffffffu));
    assert(psx_gte_read_data(&gte, 31u, &value));
    assert(value == 7u);
    assert(psx_gte_write_data(&gte, 30u, 0xffffffffu));
    assert(psx_gte_read_data(&gte, 31u, &value));
    assert(value == 31u);
    assert(psx_gte_write_data(&gte, 30u, 0u));
    assert(psx_gte_read_data(&gte, 31u, &value));
    assert(value == 32u);

    assert(psx_gte_write_data(&gte, 12u, 0x00010002u));
    assert(psx_gte_write_data(&gte, 13u, 0x00030004u));
    assert(psx_gte_write_data(&gte, 14u, 0x00050006u));
    assert(psx_gte_write_data(&gte, 15u, 0x00070008u));
    assert(psx_gte_read_data(&gte, 12u, &value));
    assert(value == 0x00030004u);
    assert(psx_gte_read_data(&gte, 13u, &value));
    assert(value == 0x00050006u);
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert(value == 0x00070008u);
    assert(psx_gte_read_data(&gte, 15u, &value));
    assert(value == 0x00070008u);

    assert(psx_gte_write_control(&gte, 31u, 0xffffffffu));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert(value == 0xfffff000u);

    assert(!psx_gte_read_data(&gte, 32u, &value));
    assert(!psx_gte_write_data(&gte, 32u, 0u));
    assert(!psx_gte_read_control(&gte, 32u, &value));
    assert(!psx_gte_write_control(&gte, 32u, 0u));
    assert(!psx_gte_read_data(0, 0u, &value));
    assert(!psx_gte_write_control(0, 0u, 0u));

    return 0;
}
