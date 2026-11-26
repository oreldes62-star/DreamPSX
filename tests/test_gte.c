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
    assert(value == 31u);

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
    /* NCLIP: counter-clockwise, clockwise and degenerate triangles. */
    assert(psx_gte_write_data(&gte, 12u, 0x00000000u)); /* (0,0) */
    assert(psx_gte_write_data(&gte, 13u, 0x00000064u)); /* (100,0) */
    assert(psx_gte_write_data(&gte, 14u, 0x00640000u)); /* (0,100) */
    assert(psx_gte_execute(&gte, 0x4b400006u));
    assert(psx_gte_read_data(&gte, 24u, &value));
    assert(value == 10000u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    assert(psx_gte_write_data(&gte, 13u, 0x00640000u));
    assert(psx_gte_write_data(&gte, 14u, 0x00000064u));
    assert(psx_gte_execute(&gte, 0x4b400006u));
    assert(psx_gte_read_data(&gte, 24u, &value));
    assert(value == (uint32_t)-10000);

    assert(psx_gte_write_data(&gte, 13u, 0x00320032u));
    assert(psx_gte_write_data(&gte, 14u, 0x00640064u));
    assert(psx_gte_execute(&gte, 0x4b400006u));
    assert(psx_gte_read_data(&gte, 24u, &value));
    assert(value == 0u);

    /* Extreme signed 16-bit coordinates exercise MAC0 overflow reporting. */
    assert(psx_gte_write_data(&gte, 12u, 0x7fff7fffu));
    assert(psx_gte_write_data(&gte, 13u, 0x80008000u));
    assert(psx_gte_write_data(&gte, 14u, 0x7fff8000u));
    assert(psx_gte_execute(&gte, 0x4b400006u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15)) != 0u);
    assert((value & (1u << 31)) != 0u);

    /* MVMVA: identity rotation, V0 plus a translation vector. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u)); /* RT11=4096 */
    assert(psx_gte_write_control(&gte, 1u, 0u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u)); /* RT22=4096 */
    assert(psx_gte_write_control(&gte, 3u, 0u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u)); /* RT33=4096 */
    assert(psx_gte_write_control(&gte, 5u, 10u));
    assert(psx_gte_write_control(&gte, 6u, (uint32_t)-20));
    assert(psx_gte_write_control(&gte, 7u, 30u));
    assert(psx_gte_write_data(&gte, 0u, 0x00020001u)); /* V0=(1,2,3) */
    assert(psx_gte_write_data(&gte, 1u, 3u));
    assert(psx_gte_execute(&gte, 0x48080012u)); /* MVMVA sf=1, RT, V0, TR */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 11u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == (uint32_t)-18);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 33u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 11u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == (uint32_t)-18);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 33u);

    /* sf=0 leaves the 12-bit fixed-point matrix scale unshifted. */
    assert(psx_gte_execute(&gte, 0x48006012u)); /* RT, V0, no translation */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 4096u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 8192u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 12288u);

    /* lm=1 clamps negative IR outputs to zero while preserving MAC values. */
    assert(psx_gte_write_data(&gte, 0u, 0xffff0001u)); /* V0=(-1,-1,3) */
    assert(psx_gte_write_data(&gte, 1u, 3u));
    assert(psx_gte_execute(&gte, 0x48086412u)); /* sf=1, lm=1, RT/V0/no TR */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == (uint32_t)-1);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 0u);

    assert(!psx_gte_execute(&gte, 0x4b400007u));
    assert(!psx_gte_execute(0, 0x4b400006u));

    assert(!psx_gte_read_data(0, 0u, &value));
    assert(!psx_gte_write_control(0, 0u, 0u));

    return 0;
}
