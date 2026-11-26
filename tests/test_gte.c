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

    /* RTPS: identity projection, screen offsets and SZ FIFO output. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 1000u)); /* TRZ */
    assert(psx_gte_write_control(&gte, 24u, 160u << 16u)); /* OFX */
    assert(psx_gte_write_control(&gte, 25u, 120u << 16u)); /* OFY */
    assert(psx_gte_write_control(&gte, 26u, 200u)); /* H */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_execute(&gte, 0x4a080001u)); /* RTPS, sf=1 */
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 1000u);
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value & 0xffffu) == 160);
    assert((int16_t)(value >> 16u) == 120);

    /* RTPS projects an offset vertex using H/SZ3. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 24u, 160u << 16u));
    assert(psx_gte_write_control(&gte, 25u, 120u << 16u));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_write_data(&gte, 0u, 0x00320064u)); /* V0=(100,50,500) */
    assert(psx_gte_write_data(&gte, 1u, 500u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 500u);
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value & 0xffffu) == 199);
    assert((int16_t)(value >> 16u) == 139);

    /* RTPS near-plane divide overflow and screen-coordinate saturation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_write_data(&gte, 0u, 0x7fffu));
    assert(psx_gte_write_data(&gte, 1u, 100u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) != 0u);
    assert((value & (1u << 14u)) != 0u);
    assert((value & (1u << 31u)) != 0u);
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value & 0xffffu) == 1023);

    /* RTPS sf=0 preserves the unshifted MAC3 and derives SZ3 >> 12. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 0x1000u));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_execute(&gte, 0x4a000001u)); /* RTPS, sf=0 */
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 0x01000000u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 0x7fffu);
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 4096u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* Negative camera-space depth clamps SZ3 to zero, not to 0xffff. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, (uint32_t)-1));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 0u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) != 0u);
    assert((value & (1u << 18u)) == 0u);

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

    /* MVMVA FC translation quirk: first column is discarded from MAC,
     * while the remaining columns produce the final result. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 21u, 0x1000u));
    assert(psx_gte_write_control(&gte, 22u, 0x2000u));
    assert(psx_gte_write_control(&gte, 23u, 0x3000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u));
    assert(psx_gte_write_data(&gte, 1u, 0x300u));
    assert(psx_gte_execute(&gte, 0x48084012u)); /* sf=1, RT, V0, FC */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 512u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 768u);

    /* MVMVA selector coverage: V1, V2 and IR vector sources. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 2u, 0x00050004u)); /* V1=(4,5,6) */
    assert(psx_gte_write_data(&gte, 3u, 6u));
    assert(psx_gte_execute(&gte, 0x4808e012u)); /* sf=1, RT, V1, no TR */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 4u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 5u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 6u);

    assert(psx_gte_write_data(&gte, 4u, 0x00080007u)); /* V2=(7,8,9) */
    assert(psx_gte_write_data(&gte, 5u, 9u));
    assert(psx_gte_execute(&gte, 0x48096012u)); /* sf=1, RT, V2, no TR */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 7u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 8u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 9u);

    assert(psx_gte_write_data(&gte, 9u, 10u));
    assert(psx_gte_write_data(&gte, 10u, 11u));
    assert(psx_gte_write_data(&gte, 11u, 12u));
    assert(psx_gte_execute(&gte, 0x4809e012u)); /* sf=1, RT, IR, no TR */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 10u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 11u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 12u);

    /* Matrix selector coverage: light and color identity matrices. */
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_execute(&gte, 0x480be012u)); /* sf=1, light, IR, no TR */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 10u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 11u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 12u);
    assert(psx_gte_execute(&gte, 0x480de012u)); /* sf=1, color, IR, no TR */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 10u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 11u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 12u);

    /* Negative MAC1 overflow must set FLAG bit 27 (not bit 29). */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00007fffu)); /* RT11=32767 */
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 5u, 0x80000000u)); /* TRX=INT32_MIN */
    assert(psx_gte_write_data(&gte, 0u, 0xffff8000u)); /* V0.x=-32768 */
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_execute(&gte, 0x48000012u)); /* sf=0, RT, V0, no TR */
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 27u)) != 0u);
    assert((value & (1u << 29u)) == 0u);
    assert((value & 0x80000000u) != 0u);

    /* sf=0 leaves the 12-bit fixed-point matrix scale unshifted. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x00020001u));
    assert(psx_gte_write_data(&gte, 1u, 3u));
    assert(psx_gte_execute(&gte, 0x48006012u)); /* RT, V0, no translation */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 4096u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 8192u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 12288u);

    /* lm=1 clamps negative IR outputs to zero while preserving MAC values. */
    assert(psx_gte_write_data(&gte, 0u, 0xffffffffu)); /* V0=(-1,-1,3) */
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
