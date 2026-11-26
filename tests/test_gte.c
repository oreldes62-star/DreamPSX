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

    /* VZ0..VZ2 are signed 16-bit registers on read and write. */
    for (uint8_t index = 1u; index <= 5u; index += 2u) {
        assert(psx_gte_write_data(&gte, index, 0x12348001u));
        assert(psx_gte_read_data(&gte, index, &value));
        assert(value == 0xffff8001u);
    }

    /* OTZ and SZ0..SZ3 are unsigned 16-bit registers. */
    for (uint8_t index = 7u; index <= 19u; ++index) {
        if (index != 7u && index < 16u) continue;
        if (index > 7u && index < 16u) continue;
        assert(psx_gte_write_data(&gte, index, 0x1234ffffu));
        assert(psx_gte_read_data(&gte, index, &value));
        assert(value == 0xffffu);
    }

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
    assert(value == 8u);
    assert(psx_gte_write_data(&gte, 30u, 0xffffffffu));
    assert(psx_gte_read_data(&gte, 31u, &value));
    assert(value == 32u);
    assert(psx_gte_write_data(&gte, 30u, 0u));
    assert(psx_gte_read_data(&gte, 31u, &value));
    assert(value == 32u);
    assert(psx_gte_write_data(&gte, 30u, 0x00000001u));
    assert(psx_gte_read_data(&gte, 31u, &value) && value == 31u);
    assert(psx_gte_write_data(&gte, 30u, 0x7fffffffu));
    assert(psx_gte_read_data(&gte, 31u, &value) && value == 1u);
    assert(psx_gte_write_data(&gte, 30u, 0x80000000u));
    assert(psx_gte_read_data(&gte, 31u, &value) && value == 1u);
    assert(psx_gte_write_data(&gte, 30u, 0x00008000u));
    assert(psx_gte_read_data(&gte, 31u, &value) && value == 16u);
    assert(psx_gte_write_data(&gte, 30u, 0xffff7fffu));
    assert(psx_gte_read_data(&gte, 31u, &value) && value == 16u);
    assert(psx_gte_write_data(&gte, 30u, 0x12345678u));
    assert(psx_gte_read_data(&gte, 31u, &value) && value == 3u);
    assert(psx_gte_write_data(&gte, 30u, 0xf0000000u));
    assert(psx_gte_read_data(&gte, 31u, &value) && value == 4u);
    /* LZCR is read-only; writes must not change the calculated result. */
    assert(psx_gte_write_data(&gte, 31u, 5u));
    assert(psx_gte_read_data(&gte, 31u, &value) && value == 4u);

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

    /* H is unsigned 16-bit; DQA, ZSF3 and ZSF4 are signed 16-bit. */
    assert(psx_gte_write_control(&gte, 26u, 0x12345678u));
    assert(psx_gte_read_control(&gte, 26u, &value) && value == 0x5678u);
    assert(psx_gte_write_control(&gte, 27u, 0x12348001u));
    assert(psx_gte_read_control(&gte, 27u, &value) && value == 0xffff8001u);
    assert(psx_gte_write_control(&gte, 29u, 0x1234ffffu));
    assert(psx_gte_read_control(&gte, 29u, &value) && value == 0xffffffffu);
    assert(psx_gte_write_control(&gte, 30u, 0x12347fffu));
    assert(psx_gte_read_control(&gte, 30u, &value) && value == 0x7fffu);

    assert(psx_gte_write_control(&gte, 31u, 0xffffffffu));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert(value == 0xfffff000u);

    /* FLAG writes ignore the summary bit and reserved low bits, then
     * recompute the summary from writable exception bits. */
    assert(psx_gte_write_control(&gte, 31u, 0x80000000u));
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);
    assert(psx_gte_write_control(&gte, 31u, 0x00000fffu));
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);
    /* IR0 saturation (FLAG bit 12) is writable but does not contribute
     * to the hardware FLAG summary bit. */
    assert(psx_gte_write_control(&gte, 31u, 0x00001000u));
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0x00001000u);
    /* Screen-coordinate saturation (FLAG bit 13) does contribute. */
    assert(psx_gte_write_control(&gte, 31u, 0x00002000u));
    assert(psx_gte_read_control(&gte, 31u, &value) &&
           value == 0x80002000u);
    assert(psx_gte_write_control(&gte, 31u, 0x40001001u));
    assert(psx_gte_read_control(&gte, 31u, &value) &&
           value == 0xc0001000u);
    assert(psx_gte_write_control(&gte, 31u, 0u));
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

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

    /* RTPS IR0 lower boundary: zero is inclusive and does not saturate. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 4096u)); /* TRZ => SZ3=4096 */
    assert(psx_gte_write_control(&gte, 26u, 0u)); /* H=0 => quotient=0 */
    assert(psx_gte_write_control(&gte, 27u, 0)); /* DQA=0 */
    assert(psx_gte_write_control(&gte, 28u, 0u)); /* DQB=0 */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 8u, &value) && value == 0u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 12u)) == 0u);

    /* RTPS IR0 clamps one unit below zero and sets FLAG.12. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 4096u));
    assert(psx_gte_write_control(&gte, 26u, 0u));
    assert(psx_gte_write_control(&gte, 27u, 0));
    assert(psx_gte_write_control(&gte, 28u, 0xfffff000u)); /* DQB=-4096 */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 8u, &value) && value == 0u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 12u)) != 0u);
    /* FLAG.12 is deliberately excluded from the summary bit 31. */
    assert((value & (1u << 31u)) == 0u);

    /* RTPS IR0 upper boundary: 4096 is inclusive. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 4096u));
    assert(psx_gte_write_control(&gte, 26u, 0u));
    assert(psx_gte_write_control(&gte, 27u, 0));
    assert(psx_gte_write_control(&gte, 28u, 0x01000000u)); /* DQB=4096<<12 */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 8u, &value) && value == 4096u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 12u)) == 0u);

    /* RTPS IR0 clamps one unit above 4096 and sets FLAG.12. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 4096u));
    assert(psx_gte_write_control(&gte, 26u, 0u));
    assert(psx_gte_write_control(&gte, 27u, 0));
    assert(psx_gte_write_control(&gte, 28u, 0x01001000u)); /* DQB=4097<<12 */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 8u, &value) && value == 4096u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 12u)) != 0u);
    assert((value & (1u << 31u)) == 0u);

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

    /* UNR reciprocal reference vector: H=12345, SZ3=23456.
     * DuckStation's integer UNR path gives quotient 34492; IR1=1024
     * therefore projects to screen X=538 after the 16-bit fixed-point shift. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 23456u)); /* SZ3 */
    assert(psx_gte_write_control(&gte, 26u, 12345u)); /* H */
    assert(psx_gte_write_data(&gte, 0u, 0x00000400u)); /* V0=(1024,0,0) */
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value & 0xffffu) == 538);

    /* UNR table index 0: normalized SZ3 is exactly 0x8000. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 0x8000u));
    assert(psx_gte_write_control(&gte, 26u, 0x4000u));
    assert(psx_gte_write_control(&gte, 27u, 1u)); /* DQA */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 8u, &value) && value == 8u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) == 0u);

    /* UNR table index 256: normalized SZ3 is 0xffff. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 0xffffu));
    assert(psx_gte_write_control(&gte, 26u, 0x7fffu));
    assert(psx_gte_write_control(&gte, 27u, 1u)); /* DQA */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 8u, &value) && value == 7u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) == 0u);

    /* The UNR quotient saturates to 0x1ffff without divide-overflow FLAG. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 0x7f20u));
    assert(psx_gte_write_control(&gte, 26u, 0xfe3fu));
    assert(psx_gte_write_control(&gte, 27u, 1u)); /* DQA */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 8u, &value) && value == 31u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) == 0u);

    /* Screen X lower boundary is inclusive: -1024 does not saturate. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 1000u));
    assert(psx_gte_write_control(&gte, 24u, (uint32_t)(-67108864)));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value & 0xffffu) == -1024);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 14u)) == 0u);

    /* One unit below the lower boundary clamps to -1024 and sets FLAG.14. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 1000u));
    assert(psx_gte_write_control(&gte, 24u, (uint32_t)(-67174400)));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value & 0xffffu) == -1024);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 14u)) != 0u);
    assert((value & 0x80000000u) != 0u);

    /* Screen X upper boundary is inclusive: +1023 does not saturate. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 1000u));
    assert(psx_gte_write_control(&gte, 24u, 67043328u));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value & 0xffffu) == 1023);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 14u)) == 0u);

    /* One unit above the upper X boundary clamps to +1023 and sets FLAG.14. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 1000u));
    assert(psx_gte_write_control(&gte, 24u, 67108864u));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value & 0xffffu) == 1023);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 14u)) != 0u);
    assert((value & 0x80000000u) != 0u);

    /* Screen Y upper boundary is inclusive, then saturates above +1023. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 1000u));
    assert(psx_gte_write_control(&gte, 25u, 67043328u));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value >> 16u) == 1023);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 13u)) == 0u);

    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 1000u));
    assert(psx_gte_write_control(&gte, 25u, 67108864u));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value >> 16u) == 1023);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 13u)) != 0u);
    assert((value & 0x80000000u) != 0u);

    /* Positive MAC0 overflow during RTPS projection is reported separately. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 1000u));
    assert(psx_gte_write_control(&gte, 24u, 0x7fffffffu));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_write_data(&gte, 0u, 0x00007fffu));
    assert(psx_gte_write_data(&gte, 1u, 1000u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 16u)) != 0u);
    assert((value & (1u << 14u)) != 0u);
    assert((value & 0x80000000u) != 0u);

    /* Negative MAC0 overflow during RTPS projection is reported separately. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 1000u));
    assert(psx_gte_write_control(&gte, 24u, 0x80000000u));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_write_data(&gte, 0u, 0x00008000u));
    assert(psx_gte_write_data(&gte, 1u, 1000u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15u)) != 0u);
    assert((value & (1u << 14u)) != 0u);
    assert((value & 0x80000000u) != 0u);

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

    /* UNR zero-depth corner: H=0 and SZ3=0 triggers divide overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 26u, 0u)); /* H=0 */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) != 0u);
    assert((value & (1u << 31u)) != 0u);
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 0u);

    /* UNR divide boundary: H just below 2*SZ is valid. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 101u)); /* SZ3=101 */
    assert(psx_gte_write_control(&gte, 26u, 200u)); /* H=2*101-2 */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) == 0u);
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 101u);

    /* At H == 2*SZ the hardware reports divide overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 100u)); /* SZ3=100 */
    assert(psx_gte_write_control(&gte, 26u, 200u)); /* H=2*SZ */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) != 0u);
    assert((value & (1u << 31u)) != 0u);

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

    /* RTPT transforms V0/V1/V2 and leaves their results in FIFO order. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 24u, 160u << 16u));
    assert(psx_gte_write_control(&gte, 25u, 120u << 16u));
    assert(psx_gte_write_control(&gte, 26u, 200u));
    assert(psx_gte_write_data(&gte, 0u, 0u));          /* V0=(0,0,1000) */
    assert(psx_gte_write_data(&gte, 1u, 1000u));
    assert(psx_gte_write_data(&gte, 2u, 100u));        /* V1=(100,0,1000) */
    assert(psx_gte_write_data(&gte, 3u, 1000u));
    assert(psx_gte_write_data(&gte, 4u, 0x00640000u)); /* V2=(0,100,1000) */
    assert(psx_gte_write_data(&gte, 5u, 1000u));
    /* Seed SZ FIFO to verify that three RTPS passes shift old depth out
     * while preserving the previous newest entry in SZ0. */
    assert(psx_gte_write_data(&gte, 16u, 111u));
    assert(psx_gte_write_data(&gte, 17u, 222u));
    assert(psx_gte_write_data(&gte, 18u, 333u));
    assert(psx_gte_write_data(&gte, 19u, 444u));
    assert(psx_gte_execute(&gte, 0x4a080030u)); /* RTPT, sf=1 */
    assert(psx_gte_read_data(&gte, 12u, &value));
    assert((int16_t)(value & 0xffffu) == 160);
    assert((int16_t)(value >> 16u) == 120);
    assert(psx_gte_read_data(&gte, 13u, &value));
    assert((int16_t)(value & 0xffffu) == 179);
    assert((int16_t)(value >> 16u) == 120);
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((int16_t)(value & 0xffffu) == 160);
    assert((int16_t)(value >> 16u) == 139);
    /* SXYP is the readable alias of the newest SXY FIFO entry. */
    assert(psx_gte_read_data(&gte, 15u, &value) && value == 0x008b00a0u);
    assert(psx_gte_read_data(&gte, 17u, &value) && value == 1000u);
    assert(psx_gte_read_data(&gte, 18u, &value) && value == 1000u);
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 1000u);
    /* RTPT leaves MAC/IR registers at the final vertex and shifts all
     * four depth entries, including the previous SZ3, through the FIFO. */
    assert(psx_gte_read_data(&gte, 16u, &value) && value == 444u);
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 100u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 1000u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 100u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 1000u);

    /* RTPT must retain exceptions raised by earlier vertices even when
     * the final vertex completes without a divide overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 26u, 200u)); /* H */
    assert(psx_gte_write_data(&gte, 0u, 0x00007fffu)); /* V0=(32767,0,0) */
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_write_data(&gte, 2u, 0u));       /* V1=(0,0,1000) */
    assert(psx_gte_write_data(&gte, 3u, 1000u));
    assert(psx_gte_write_data(&gte, 4u, 0u));       /* V2=(0,0,1000) */
    assert(psx_gte_write_data(&gte, 5u, 1000u));
    assert(psx_gte_execute(&gte, 0x4a080030u)); /* RTPT, sf=1 */
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 1000u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) != 0u); /* V0 divide overflow retained */
    assert((value & (1u << 16u)) != 0u); /* V0 MAC0 positive overflow */
    assert((value & (1u << 14u)) != 0u); /* V0 screen X saturation */
    assert((value & (1u << 31u)) != 0u); /* summary retained */

    /* AVSZ3 averages the last three SZ FIFO entries into OTZ. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 29u, 0x1000u)); /* ZSF3=1.0 */
    assert(psx_gte_write_data(&gte, 17u, 1000u));
    assert(psx_gte_write_data(&gte, 18u, 2000u));
    assert(psx_gte_write_data(&gte, 19u, 3000u));
    assert(psx_gte_execute(&gte, 0x4a00002du));
    assert(psx_gte_read_data(&gte, 24u, &value) && value == 6000u * 4096u);
    assert(psx_gte_read_data(&gte, 7u, &value) && value == 6000u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* AVSZ4 includes SZ0 and uses ZSF4. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 30u, 0x1000u)); /* ZSF4=1.0 */
    assert(psx_gte_write_data(&gte, 16u, 500u));
    assert(psx_gte_write_data(&gte, 17u, 1000u));
    assert(psx_gte_write_data(&gte, 18u, 2000u));
    assert(psx_gte_write_data(&gte, 19u, 3000u));
    assert(psx_gte_execute(&gte, 0x4a00002eu));
    assert(psx_gte_read_data(&gte, 24u, &value) && value == 6500u * 4096u);
    assert(psx_gte_read_data(&gte, 7u, &value) && value == 6500u);

    /* AVSZ3 clamps negative OTZ and reports the OTZ saturation flag. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 29u, (uint32_t)-0x1000));
    assert(psx_gte_write_data(&gte, 17u, 1000u));
    assert(psx_gte_write_data(&gte, 18u, 2000u));
    assert(psx_gte_write_data(&gte, 19u, 3000u));
    assert(psx_gte_execute(&gte, 0x4a00002du));
    assert(psx_gte_read_data(&gte, 7u, &value) && value == 0u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 18u)) != 0u);
    assert((value & (1u << 31u)) != 0u);

    /* AVSZ4 reports MAC0 positive overflow and clamps OTZ to 0xffff. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 30u, 0x7fffu));
    assert(psx_gte_write_data(&gte, 16u, 0xffffu));
    assert(psx_gte_write_data(&gte, 17u, 0xffffu));
    assert(psx_gte_write_data(&gte, 18u, 0xffffu));
    assert(psx_gte_write_data(&gte, 19u, 0xffffu));
    assert(psx_gte_execute(&gte, 0x4a00002eu));
    assert(psx_gte_read_data(&gte, 7u, &value) && value == 0xffffu);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 16u)) != 0u);
    assert((value & (1u << 18u)) != 0u);
    assert((value & (1u << 31u)) != 0u);

    /* AVSZ4 also reports negative MAC0 overflow and clamps OTZ to zero. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 30u, 0x8000u)); /* ZSF4=-32768 */
    assert(psx_gte_write_data(&gte, 16u, 0xffffu));
    assert(psx_gte_write_data(&gte, 17u, 0xffffu));
    assert(psx_gte_write_data(&gte, 18u, 0xffffu));
    assert(psx_gte_write_data(&gte, 19u, 0xffffu));
    assert(psx_gte_execute(&gte, 0x4a00002eu));
    assert(psx_gte_read_data(&gte, 7u, &value) && value == 0u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15u)) != 0u); /* MAC0 negative overflow */
    assert((value & (1u << 18u)) != 0u); /* OTZ lower saturation */
    assert((value & (1u << 31u)) != 0u); /* FLAG summary */

    /* AVSZ4 stays below positive MAC0 overflow at INT32_MAX-1. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 30u, 0x7fffu)); /* ZSF4=32767 */
    assert(psx_gte_write_data(&gte, 16u, 65535u));
    assert(psx_gte_write_data(&gte, 17u, 3u));
    assert(psx_gte_execute(&gte, 0x4a00002eu));
    assert(psx_gte_read_data(&gte, 24u, &value) &&
           value == 0x7ffffffeu);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 16u)) == 0u); /* no positive MAC0 overflow */

    /* Raising the sum by one crosses the signed 32-bit MAC0 boundary. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 30u, 0x7fffu));
    assert(psx_gte_write_data(&gte, 16u, 65535u));
    assert(psx_gte_write_data(&gte, 17u, 4u));
    assert(psx_gte_execute(&gte, 0x4a00002eu));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 16u)) != 0u); /* positive MAC0 overflow */

    /* AVSZ4 reaches INT32_MIN exactly without negative MAC0 overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 30u, 0x8000u)); /* ZSF4=-32768 */
    assert(psx_gte_write_data(&gte, 16u, 0xffffu));
    assert(psx_gte_write_data(&gte, 17u, 1u));
    assert(psx_gte_execute(&gte, 0x4a00002eu));
    assert(psx_gte_read_data(&gte, 24u, &value) &&
           value == 0x80000000u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15u)) == 0u); /* exact lower bound */

    /* One additional depth unit crosses below INT32_MIN. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 30u, 0x8000u)); /* ZSF4=-32768 */
    assert(psx_gte_write_data(&gte, 16u, 0xffffu));
    assert(psx_gte_write_data(&gte, 17u, 2u));
    assert(psx_gte_execute(&gte, 0x4a00002eu));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15u)) != 0u); /* negative MAC0 overflow */

    /* AVSZ3 reaches INT32_MIN exactly without negative MAC0 overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 29u, 0x8000u)); /* ZSF3=-32768 */
    assert(psx_gte_write_data(&gte, 17u, 0xffffu));
    assert(psx_gte_write_data(&gte, 18u, 1u));
    assert(psx_gte_write_data(&gte, 19u, 0u));
    assert(psx_gte_execute(&gte, 0x4a00002du));
    assert(psx_gte_read_data(&gte, 24u, &value) &&
           value == 0x80000000u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15u)) == 0u); /* exact lower bound */

    /* AVSZ3 crosses below INT32_MIN with one additional depth unit. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 29u, 0x8000u));
    assert(psx_gte_write_data(&gte, 17u, 0xffffu));
    assert(psx_gte_write_data(&gte, 18u, 2u));
    assert(psx_gte_write_data(&gte, 19u, 0u));
    assert(psx_gte_execute(&gte, 0x4a00002du));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15u)) != 0u); /* negative MAC0 overflow */

    /* AVSZ3 stays below positive MAC0 overflow at INT32_MAX-1. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 29u, 0x7fffu)); /* ZSF3=32767 */
    assert(psx_gte_write_data(&gte, 17u, 65535u));
    assert(psx_gte_write_data(&gte, 18u, 3u));
    assert(psx_gte_write_data(&gte, 19u, 0u));
    assert(psx_gte_execute(&gte, 0x4a00002du));
    assert(psx_gte_read_data(&gte, 24u, &value) &&
           value == 0x7ffffffeu);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 16u)) == 0u); /* no positive MAC0 overflow */

    /* One additional depth unit crosses above INT32_MAX. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 29u, 0x7fffu));
    assert(psx_gte_write_data(&gte, 17u, 65535u));
    assert(psx_gte_write_data(&gte, 18u, 4u));
    assert(psx_gte_write_data(&gte, 19u, 0u));
    assert(psx_gte_execute(&gte, 0x4a00002du));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 16u)) != 0u); /* positive MAC0 overflow */

    /* RTPS projection MAC0 accepts the exact positive signed boundary. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 24u, 0x7fffffffu)); /* OFX */
    assert(psx_gte_write_control(&gte, 28u, 0x7fffffffu)); /* DQB: final MAC0 */
    assert(psx_gte_write_control(&gte, 26u, 0u)); /* H=0, quotient=0 */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 1000u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 24u, &value) &&
           value == 0x7fffffffu);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 16u)) == 0u); /* exact upper bound */
    assert((value & (1u << 14u)) != 0u); /* projected SX clamps */

    /* RTPS projection MAC0 accepts the exact negative signed boundary. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 24u, 0x80000000u)); /* OFX */
    assert(psx_gte_write_control(&gte, 28u, 0x80000000u)); /* DQB: final MAC0 */
    assert(psx_gte_write_control(&gte, 26u, 0u)); /* H=0, quotient=0 */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 1000u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 24u, &value) &&
           value == 0x80000000u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15u)) == 0u); /* exact lower bound */
    assert((value & (1u << 14u)) != 0u); /* projected SX clamps */

    /* RTPS projection MAC0: positive overflow by a nonzero quotient. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 24u, 0x7fffffffu)); /* OFX */
    assert(psx_gte_write_control(&gte, 26u, 1u)); /* H=1 */
    assert(psx_gte_write_data(&gte, 0u, 1u)); /* IR1=1 */
    assert(psx_gte_write_data(&gte, 1u, 4096u)); /* SZ=4096 */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 16u)) != 0u); /* X projection overflows */

    /* RTPS projection MAC0: negative overflow by a nonzero quotient. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 24u, 0x80000000u)); /* OFX */
    assert(psx_gte_write_control(&gte, 26u, 1u)); /* H=1 */
    assert(psx_gte_write_data(&gte, 0u, 0xffffffffu)); /* IR1=-1 */
    assert(psx_gte_write_data(&gte, 1u, 4096u)); /* SZ=4096 */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15u)) != 0u); /* X projection underflows */

    /* RTPS projection Y MAC0: positive overflow is reported independently. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 25u, 0x7fffffffu)); /* OFY */
    assert(psx_gte_write_control(&gte, 26u, 1u)); /* H=1 */
    assert(psx_gte_write_data(&gte, 0u, 0x00010000u)); /* IR2=1 */
    assert(psx_gte_write_data(&gte, 1u, 4096u)); /* SZ=4096 */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 16u)) != 0u); /* Y projection overflows */

    /* RTPS projection Y MAC0: negative overflow is reported independently. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 25u, 0x80000000u)); /* OFY */
    assert(psx_gte_write_control(&gte, 26u, 1u)); /* H=1 */
    assert(psx_gte_write_data(&gte, 0u, 0xffff0000u)); /* IR2=-1 */
    assert(psx_gte_write_data(&gte, 1u, 4096u)); /* SZ=4096 */
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 15u)) != 0u); /* Y projection underflows */

    /* RTPS screen X clamp is independent from MAC0 overflow flags. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 24u, 0x7ffff000u)); /* OFX */
    assert(psx_gte_write_control(&gte, 26u, 0u)); /* H=0 */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 4096u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value) &&
           (int16_t)(value & 0xffffu) == 1023);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 14u)) != 0u); /* SX saturation */
    assert((value & (1u << 16u)) == 0u); /* no MAC0 overflow */

    /* RTPS screen Y clamp is independent from MAC0 overflow flags. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 25u, 0x7ffff000u)); /* OFY */
    assert(psx_gte_write_control(&gte, 26u, 0u)); /* H=0 */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 4096u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value) &&
           (int16_t)(value >> 16u) == 1023);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 13u)) != 0u); /* SY saturation */
    assert((value & (1u << 16u)) == 0u); /* no MAC0 overflow */

    /* RTPS negative screen X clamp reaches -1024 without MAC0 overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 24u, (uint32_t)(-1025 * 65536))); /* OFX */
    assert(psx_gte_write_control(&gte, 26u, 0u)); /* H=0 */
    assert(psx_gte_write_control(&gte, 28u, (uint32_t)(-1025 * 65536))); /* DQB */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 4096u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value) &&
           (int16_t)(value & 0xffffu) == -1024);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 14u)) != 0u); /* SX saturation */
    assert((value & (1u << 15u)) == 0u); /* no MAC0 overflow */

    /* RTPS negative screen Y clamp reaches -1024 without MAC0 overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x1000u));
    assert(psx_gte_write_control(&gte, 2u, 0x1000u));
    assert(psx_gte_write_control(&gte, 4u, 0x1000u));
    assert(psx_gte_write_control(&gte, 25u, (uint32_t)(-1025 * 65536)); /* OFY */
    assert(psx_gte_write_control(&gte, 26u, 0u)); /* H=0 */
    assert(psx_gte_write_control(&gte, 28u, (uint32_t)(-1025 * 65536)); /* DQB */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 4096u));
    assert(psx_gte_execute(&gte, 0x4a080001u));
    assert(psx_gte_read_data(&gte, 14u, &value) &&
           (int16_t)(value >> 16u) == -1024);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 13u)) != 0u); /* SY saturation */
    assert((value & (1u << 15u)) == 0u); /* no MAC0 overflow */

    /* The signed 44-bit MAC boundary is inclusive at both ends.
     * TRX*4096 is 0x7ffffffff000; adding 4095 reaches the
     * largest representable value without raising MAC1 overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 1u)); /* RT[0][0] */
    assert(psx_gte_write_control(&gte, 5u, 0x7fffffffu)); /* TRX */
    assert(psx_gte_write_data(&gte, 0u, 4095u)); /* V0.x */
    assert(psx_gte_execute(&gte, 0x4a000012u)); /* MVMVA, sf=0 */
    assert(psx_gte_read_data(&gte, 25u, &value));
    assert(value == 0xffffffffu);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 30u)) == 0u);

    /* One additional MAC unit crosses the positive 44-bit boundary. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 1u));
    assert(psx_gte_write_control(&gte, 5u, 0x7fffffffu));
    assert(psx_gte_write_data(&gte, 0u, 4096u));
    assert(psx_gte_execute(&gte, 0x4a000012u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 30u)) != 0u);

    /* -2^43 is representable; subtracting one more unit is overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 1u));
    assert(psx_gte_write_control(&gte, 5u, 0x80000000u)); /* TRX */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_execute(&gte, 0x4a000012u));
    assert(psx_gte_read_data(&gte, 25u, &value));
    assert(value == 0u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 27u)) == 0u);

    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 1u));
    assert(psx_gte_write_control(&gte, 5u, 0x80000000u));
    assert(psx_gte_write_data(&gte, 0u, 0xffffffffu)); /* V0.x=-1 */
    assert(psx_gte_execute(&gte, 0x4a000012u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 27u)) != 0u);

    /* MVMVA checks positive and negative 44-bit MAC1 overflow. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00007fffu)); /* RT[0][0] */
    assert(psx_gte_write_control(&gte, 5u, 0x7fffffffu)); /* TRX */
    assert(psx_gte_write_data(&gte, 0u, 0x00007fffu)); /* V0.x=32767 */
    assert(psx_gte_execute(&gte, 0x4a000012u)); /* MVMVA, sf=0 */
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 30u)) != 0u); /* MAC1 positive overflow */
    assert((value & (1u << 31u)) != 0u); /* FLAG summary */

    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 5u, 0x80000000u)); /* TRX */
    assert(psx_gte_write_data(&gte, 0u, 0x00008000u)); /* V0.x=-32768 */
    assert(psx_gte_execute(&gte, 0x4a000012u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 27u)) != 0u); /* MAC1 negative overflow */
    assert((value & (1u << 31u)) != 0u); /* FLAG summary */

    /* MVMVA positive/negative 44-bit overflow on MAC2. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 2u, 0x00007fffu)); /* RT[1][1] */
    assert(psx_gte_write_control(&gte, 6u, 0x7fffffffu)); /* TRY */
    assert(psx_gte_write_data(&gte, 0u, 0x7fff0000u)); /* V0.y=32767 */
    assert(psx_gte_execute(&gte, 0x4a000012u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 28u)) != 0u);
    assert((value & (1u << 31u)) != 0u);

    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 2u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 6u, 0x80000000u));
    assert(psx_gte_write_data(&gte, 0u, 0x80000000u)); /* V0.y=-32768 */
    assert(psx_gte_execute(&gte, 0x4a000012u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 25u)) != 0u);
    assert((value & (1u << 31u)) != 0u);

    /* MVMVA positive/negative 44-bit overflow on MAC3. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 4u, 0x00007fffu)); /* RT[2][2] */
    assert(psx_gte_write_control(&gte, 7u, 0x7fffffffu)); /* TRZ */
    assert(psx_gte_write_data(&gte, 1u, 0x00007fffu)); /* V0.z=32767 */
    assert(psx_gte_execute(&gte, 0x4a000012u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 26u)) != 0u);
    assert((value & (1u << 31u)) != 0u);

    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 4u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 7u, 0x80000000u));
    assert(psx_gte_write_data(&gte, 1u, 0x00008000u)); /* V0.z=-32768 */
    assert(psx_gte_execute(&gte, 0x4a000012u));
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 23u)) != 0u);
    assert((value & (1u << 31u)) != 0u);

    /* SQR squares IR1-IR3, with sf=1 shifting the MAC by 12. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_data(&gte, 9u, 3u));
    assert(psx_gte_write_data(&gte, 10u, (uint32_t)-4));
    assert(psx_gte_write_data(&gte, 11u, 100u));
    assert(psx_gte_execute(&gte, 0x4a080028u)); /* SQR, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 2u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 2u);

    /* SQR sf=0 leaves the unshifted squared values in MAC1-3. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_data(&gte, 9u, 3u));
    assert(psx_gte_write_data(&gte, 10u, (uint32_t)-4));
    assert(psx_gte_write_data(&gte, 11u, 100u));
    assert(psx_gte_execute(&gte, 0x4a000028u)); /* SQR, sf=0 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 9u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 16u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 10000u);

    /* OP computes the IR cross-product scaled by RT diagonal entries. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 9u, 1u));
    assert(psx_gte_write_data(&gte, 10u, 2u));
    assert(psx_gte_write_data(&gte, 11u, 3u));
    assert(psx_gte_execute(&gte, 0x4a08000cu)); /* OP, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 1u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == (uint32_t)-2);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 1u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 1u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == (uint32_t)-2);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 1u);

    /* OP sf=0 exposes the fixed-point cross-product before shifting.
     * Restore IR inputs because the preceding OP updated IR1-IR3. */
    assert(psx_gte_write_data(&gte, 9u, 1u));
    assert(psx_gte_write_data(&gte, 10u, 2u));
    assert(psx_gte_write_data(&gte, 11u, 3u));
    assert(psx_gte_execute(&gte, 0x4a00000cu)); /* OP, sf=0 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 4096u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == (uint32_t)-8192);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 4096u);

    /* GPF multiplies IR0 by each IR channel with fixed-point shift. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_data(&gte, 8u, 0x1000u)); /* IR0=1.0 */
    assert(psx_gte_write_data(&gte, 9u, 2u));
    assert(psx_gte_write_data(&gte, 10u, (uint32_t)-3));
    assert(psx_gte_write_data(&gte, 11u, 4u));
    assert(psx_gte_execute(&gte, 0x4a08003du)); /* GPF, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 2u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == (uint32_t)-3);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 4u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 2u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == (uint32_t)-3);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 4u);

    /* GPF sf=0 retains the raw product in MAC1-3. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_data(&gte, 8u, 2u));
    assert(psx_gte_write_data(&gte, 9u, 2u));
    assert(psx_gte_write_data(&gte, 10u, (uint32_t)-3));
    assert(psx_gte_write_data(&gte, 11u, 4u));
    assert(psx_gte_execute(&gte, 0x4a00003du)); /* GPF, sf=0 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 4u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == (uint32_t)-6);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 8u);

    /* GPL adds IR0*IR to the previous MAC shifted by 12 bits. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_data(&gte, 8u, 0x1000u));
    assert(psx_gte_write_data(&gte, 9u, 2u));
    assert(psx_gte_write_data(&gte, 10u, (uint32_t)-3));
    assert(psx_gte_write_data(&gte, 11u, 4u));
    assert(psx_gte_write_data(&gte, 25u, 5u));
    assert(psx_gte_write_data(&gte, 26u, (uint32_t)-6));
    assert(psx_gte_write_data(&gte, 27u, 7u));
    assert(psx_gte_execute(&gte, 0x4a08003eu)); /* GPL, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 7u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == (uint32_t)-9);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 11u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 7u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == (uint32_t)-9);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 11u);

    /* GPL sf=0 exposes the accumulated 44-bit MAC result. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_data(&gte, 8u, 0x1000u));
    assert(psx_gte_write_data(&gte, 9u, 2u));
    assert(psx_gte_write_data(&gte, 10u, (uint32_t)-3));
    assert(psx_gte_write_data(&gte, 11u, 4u));
    assert(psx_gte_write_data(&gte, 25u, 5u));
    assert(psx_gte_write_data(&gte, 26u, (uint32_t)-6));
    assert(psx_gte_write_data(&gte, 27u, 7u));
    assert(psx_gte_execute(&gte, 0x4a00003eu)); /* GPL, sf=0 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 8197u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == (uint32_t)-12294);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 16391u);

    /* NCS: identity light/color matrices preserve the normal and push RGB. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u)); /* normal=(256,512,768) */
    assert(psx_gte_write_data(&gte, 1u, 768u));
    assert(psx_gte_write_data(&gte, 6u, 0xab000000u)); /* CODE */
    assert(psx_gte_execute(&gte, 0x4a08001eu)); /* NCS, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 256u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 512u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 768u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 256u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 512u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 768u);
    assert(psx_gte_read_data(&gte, 22u, &value) &&
           value == 0xab302010u); /* RGB=(16,32,48), CODE=AB */

    /* NCCS: light/color stages followed by RGBC modulation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u));
    assert(psx_gte_write_data(&gte, 1u, 768u));
    assert(psx_gte_write_data(&gte, 6u, 0xab404040u)); /* RGB=64, CODE=AB */
    assert(psx_gte_execute(&gte, 0x4a08001bu)); /* NCCS, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 64u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 128u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 192u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 64u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 128u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 192u);
    assert(psx_gte_read_data(&gte, 22u, &value) &&
           value == 0xab0c0804u); /* RGB=(4,8,12), CODE=AB */

    /* NCS: background color is added before RGB conversion. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 13u, 16u));
    assert(psx_gte_write_control(&gte, 14u, 32u));
    assert(psx_gte_write_control(&gte, 15u, 48u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u));
    assert(psx_gte_write_data(&gte, 1u, 768u));
    assert(psx_gte_write_data(&gte, 6u, 0xab000000u));
    assert(psx_gte_execute(&gte, 0x4a08001eu));
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 272u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 544u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 816u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0xab332211u);

    /* NCCS: background color participates before RGBC modulation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 13u, 16u));
    assert(psx_gte_write_control(&gte, 14u, 32u));
    assert(psx_gte_write_control(&gte, 15u, 48u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u));
    assert(psx_gte_write_data(&gte, 1u, 768u));
    assert(psx_gte_write_data(&gte, 6u, 0xab808080u));
    assert(psx_gte_execute(&gte, 0x4a08001bu));
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 136u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 272u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 408u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0xab191108u);

    /* NCCT: process V0/V1/V2 and push three RGB results in order. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u)); /* V0=(256,512,768) */
    assert(psx_gte_write_data(&gte, 1u, 768u));
    assert(psx_gte_write_data(&gte, 2u, 0x01000080u)); /* V1=(128,256,384) */
    assert(psx_gte_write_data(&gte, 3u, 384u));
    assert(psx_gte_write_data(&gte, 4u, 0x00800040u)); /* V2=(64,128,192) */
    assert(psx_gte_write_data(&gte, 5u, 192u));
    assert(psx_gte_write_data(&gte, 6u, 0xab404040u));
    assert(psx_gte_execute(&gte, 0x4a08003fu)); /* NCCT, sf=1 */
    assert(psx_gte_read_data(&gte, 20u, &value) && value == 0xab0c0804u);
    assert(psx_gte_read_data(&gte, 21u, &value) && value == 0xab060402u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0xab030201u);
    assert(psx_gte_read_data(&gte, 0u, &value) && value == 0x02000100u);
    assert(psx_gte_read_data(&gte, 1u, &value) && value == 768u);

    /* NCT: three NCS lighting results, without RGBC modulation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u));
    assert(psx_gte_write_data(&gte, 1u, 768u));
    assert(psx_gte_write_data(&gte, 2u, 0x01000080u));
    assert(psx_gte_write_data(&gte, 3u, 384u));
    assert(psx_gte_write_data(&gte, 4u, 0x00800040u));
    assert(psx_gte_write_data(&gte, 5u, 192u));
    assert(psx_gte_write_data(&gte, 6u, 0xab000000u));
    assert(psx_gte_execute(&gte, 0x4a080020u)); /* NCT, sf=1 */
    assert(psx_gte_read_data(&gte, 20u, &value) && value == 0xab302010u);
    assert(psx_gte_read_data(&gte, 21u, &value) && value == 0xab181008u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0xab0c0804u);
    assert(psx_gte_read_data(&gte, 0u, &value) && value == 0x02000100u);
    assert(psx_gte_read_data(&gte, 1u, &value) && value == 768u);

    /* DPCS: interpolate the input RGB color halfway toward far color. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 21u, 0x1000u));
    assert(psx_gte_write_control(&gte, 22u, 0x1000u));
    assert(psx_gte_write_control(&gte, 23u, 0x1000u));
    assert(psx_gte_write_data(&gte, 6u, 0x00808080u));
    assert(psx_gte_write_data(&gte, 8u, 0x0800u));
    assert(psx_gte_execute(&gte, 0x4a080010u)); /* DPCS, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 3072u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 3072u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 3072u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0x00c0c0c0u);

    /* INTPL: interpolate IR vector halfway toward far color. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 21u, 0x1000u));
    assert(psx_gte_write_control(&gte, 22u, 0x1000u));
    assert(psx_gte_write_control(&gte, 23u, 0x1000u));
    assert(psx_gte_write_data(&gte, 9u, 0u));
    assert(psx_gte_write_data(&gte, 10u, 0u));
    assert(psx_gte_write_data(&gte, 11u, 0u));
    assert(psx_gte_write_data(&gte, 8u, 0x0800u));
    assert(psx_gte_execute(&gte, 0x4a080011u)); /* INTPL, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 2048u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 2048u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 2048u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0x00808080u);

    /* DPCT: consume RGB0, RGB1, RGB2 in order and push three results. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 21u, 0x1000u));
    assert(psx_gte_write_control(&gte, 22u, 0x1000u));
    assert(psx_gte_write_control(&gte, 23u, 0x1000u));
    assert(psx_gte_write_data(&gte, 6u, 0xabu << 24u));
    assert(psx_gte_write_data(&gte, 8u, 0u)); /* IR0=0 preserves input colors */
    assert(psx_gte_write_data(&gte, 20u, 0x00112233u));
    assert(psx_gte_write_data(&gte, 21u, 0x00445566u));
    assert(psx_gte_write_data(&gte, 22u, 0x00778899u));
    assert(psx_gte_execute(&gte, 0x4a08002au)); /* DPCT, sf=1 */
    assert(psx_gte_read_data(&gte, 20u, &value) && value == 0xab112233u);
    assert(psx_gte_read_data(&gte, 21u, &value) && value == 0xab445566u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0xab778899u);

    /* DCPL: light/color transform, RGBC modulation and far-color cue. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 21u, 0x1000u));
    assert(psx_gte_write_control(&gte, 22u, 0x1000u));
    assert(psx_gte_write_control(&gte, 23u, 0x1000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u));
    assert(psx_gte_write_data(&gte, 1u, 768u));
    assert(psx_gte_write_data(&gte, 6u, 0xab404040u));
    assert(psx_gte_write_data(&gte, 8u, 0x0800u));
    assert(psx_gte_execute(&gte, 0x4a080029u)); /* DCPL, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 2080u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 2112u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 2144u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0xab868482u);

    /* CC: color matrix followed by RGBC modulation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 6u, 0x00808080u));
    assert(psx_gte_write_data(&gte, 9u, 0x1000u));
    assert(psx_gte_write_data(&gte, 10u, 0x0800u));
    assert(psx_gte_write_data(&gte, 11u, 0x0400u));
    assert(psx_gte_execute(&gte, 0x4a08001cu)); /* CC, sf=1, lm=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 2048u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 1024u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 512u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 2048u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 1024u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 512u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0x00204080u);

    /* CC edge case: lm clamps negative matrix output; positive IR remains valid. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 6u, 0x00ffffffu));
    assert(psx_gte_write_data(&gte, 9u, 0xfffff000u)); /* IR1=-4096 */
    assert(psx_gte_write_data(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 11u, 0x00001000u));
    assert(psx_gte_execute(&gte, 0x4a08041cu)); /* CC, sf=1, lm=1 */
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 4080u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 4080u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0x00ffff00u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & 0x80000000u) != 0u);

    /* CDP: color matrix, RGBC modulation and far-color interpolation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 21u, 0x1000u));
    assert(psx_gte_write_control(&gte, 22u, 0x1000u));
    assert(psx_gte_write_control(&gte, 23u, 0x1000u));
    assert(psx_gte_write_data(&gte, 6u, 0xab404040u));
    assert(psx_gte_write_data(&gte, 8u, 0x0800u));
    assert(psx_gte_write_data(&gte, 9u, 256u));
    assert(psx_gte_write_data(&gte, 10u, 512u));
    assert(psx_gte_write_data(&gte, 11u, 768u));
    assert(psx_gte_execute(&gte, 0x4a080014u)); /* CDP, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 2080u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 2112u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 2144u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0xab868482u);

    /* NCDS: normal/light/color pipeline followed by far-color cue. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 21u, 0x1000u));
    assert(psx_gte_write_control(&gte, 22u, 0x1000u));
    assert(psx_gte_write_control(&gte, 23u, 0x1000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u));
    assert(psx_gte_write_data(&gte, 1u, 768u));
    assert(psx_gte_write_data(&gte, 6u, 0xab404040u));
    assert(psx_gte_write_data(&gte, 8u, 0x0800u));
    assert(psx_gte_execute(&gte, 0x4a080013u)); /* NCDS, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 2080u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 2112u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 2144u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0xab868482u);

    /* NCDT: process V0/V1/V2 and preserve the three FIFO results. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 21u, 0x1000u));
    assert(psx_gte_write_control(&gte, 22u, 0x1000u));
    assert(psx_gte_write_control(&gte, 23u, 0x1000u));
    assert(psx_gte_write_data(&gte, 0u, 0x02000100u));
    assert(psx_gte_write_data(&gte, 1u, 768u));
    assert(psx_gte_write_data(&gte, 2u, 0x01000200u));
    assert(psx_gte_write_data(&gte, 3u, 128u));
    assert(psx_gte_write_data(&gte, 4u, 0x01000080u));
    assert(psx_gte_write_data(&gte, 5u, 512u));
    assert(psx_gte_write_data(&gte, 6u, 0xab404040u));
    assert(psx_gte_write_data(&gte, 8u, 0x0800u));
    assert(psx_gte_execute(&gte, 0x4a080016u)); /* NCDT, sf=1 */
    assert(psx_gte_read_data(&gte, 20u, &value) && value == 0xab868482u);
    assert(psx_gte_read_data(&gte, 21u, &value) && value == 0xab818284u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0xab848281u);
    assert(psx_gte_read_data(&gte, 0u, &value) && value == 0x02000100u);
    assert(psx_gte_read_data(&gte, 1u, &value) && value == 768u);

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

    /* Independent PCSX-Redux cases: V2 plus BK, light matrix, and
     * light-color matrix with IR input plus BK translation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 13u, 1000u));
    assert(psx_gte_write_control(&gte, 14u, 2000u));
    assert(psx_gte_write_control(&gte, 15u, 3000u));
    assert(psx_gte_write_data(&gte, 4u, 0x00c80064u)); /* V2=(100,200,300) */
    assert(psx_gte_write_data(&gte, 5u, 300u));
    assert(psx_gte_execute(&gte, 0x48092012u)); /* sf=1, RT, V2, BK */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 1100u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 2200u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 3300u);

    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u)); /* LL33 */
    assert(psx_gte_write_data(&gte, 0u, 0x00c80064u)); /* V0=(100,200,4096) */
    assert(psx_gte_write_data(&gte, 1u, 0x1000u));
    assert(psx_gte_execute(&gte, 0x480a6012u)); /* sf=1, LL, V0, no translation */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 0x1000u);

    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 13u, 100u));
    assert(psx_gte_write_control(&gte, 14u, 200u));
    assert(psx_gte_write_control(&gte, 15u, 300u));
    assert(psx_gte_write_data(&gte, 9u, 0x1000u));
    assert(psx_gte_write_data(&gte, 10u, 0x1000u));
    assert(psx_gte_write_data(&gte, 11u, 0x1000u));
    assert(psx_gte_execute(&gte, 0x480da012u)); /* sf=1, LC, IR, BK */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 4196u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 4296u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 4396u);

    /* MVMVA reserved matrix selector 3: reproduce the documented
     * hardware "garbage matrix" result from an independent PCSX-Redux test. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x20001000u));
    assert(psx_gte_write_control(&gte, 1u, 0x40003000u));
    assert(psx_gte_write_control(&gte, 2u, 0x60005000u));
    assert(psx_gte_write_control(&gte, 3u, 0x80007000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 8u, 0x0800u)); /* IR0 */
    assert(psx_gte_write_data(&gte, 0u, 0x01000100u));
    assert(psx_gte_write_data(&gte, 1u, 0x00000100u));
    assert(psx_gte_execute(&gte, 0x480e6012u)); /* sf=1, mx=3, V0, cv=none */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 128u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 2304u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 3840u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* Independent PCSX-Redux reference: MVMVA sf=0 keeps the
     * unshifted matrix-vector products in MAC1-MAC3. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x000a000au)); /* V0=(10,10,10) */
    assert(psx_gte_write_data(&gte, 1u, 10u));
    assert(psx_gte_execute(&gte, 0x48006012u)); /* sf=0, RT, V0, no translation */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 40960u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 40960u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 40960u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 32767u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 32767u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 32767u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 24u)) != 0u); /* IR1 saturation */
    assert((value & (1u << 23u)) != 0u); /* IR2 saturation */
    assert((value & (1u << 22u)) != 0u); /* IR3 saturation */
    assert((value & 0x80000000u) != 0u); /* FLAG summary */

    /* Negative MVMVA results saturate at -32768 when lm=0. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x80008000u));
    assert(psx_gte_write_data(&gte, 1u, 0x8000u));
    assert(psx_gte_execute(&gte, 0x48006012u)); /* sf=0, lm=0 */
    assert(psx_gte_read_data(&gte, 25u, &value) &&
           value == (uint32_t)-134217728);
    assert(psx_gte_read_data(&gte, 26u, &value) &&
           value == (uint32_t)-134217728);
    assert(psx_gte_read_data(&gte, 27u, &value) &&
           value == (uint32_t)-134217728);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 0xffff8000u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 0xffff8000u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 0xffff8000u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 24u)) != 0u);
    assert((value & (1u << 23u)) != 0u);
    assert((value & (1u << 22u)) != 0u);
    assert((value & 0x80000000u) != 0u);

    /* With lm=1, negative IR results clamp to zero and set the same flags. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0xfffeffffu));
    assert(psx_gte_write_data(&gte, 1u, 0xfffdu));
    assert(psx_gte_execute(&gte, 0x48006412u)); /* sf=0, lm=1 */
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 0u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 0u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 24u)) != 0u);
    assert((value & (1u << 23u)) != 0u);
    assert((value & (1u << 22u)) != 0u);
    assert((value & 0x80000000u) != 0u);

    /* lm=1 lower IR boundary is inclusive: zero is not saturation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_execute(&gte, 0x48006412u)); /* sf=0, lm=1 */
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 0u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* lm=1 upper IR boundary is inclusive at +32767. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 1u));
    assert(psx_gte_write_data(&gte, 0u, 32767u));
    assert(psx_gte_execute(&gte, 0x48006412u));
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 32767u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* One unit above +32767 clamps and raises IR1 saturation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 2u));
    assert(psx_gte_write_data(&gte, 0u, 16384u));
    assert(psx_gte_execute(&gte, 0x48006412u));
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 32767u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 24u)) != 0u);
    assert((value & 0x80000000u) != 0u);

    /* IR2 and IR3 upper boundaries are inclusive at +32767. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 2u, 0x00010000u)); /* RT[1][1] */
    assert(psx_gte_write_control(&gte, 4u, 1u)); /* RT[2][2] */
    assert(psx_gte_write_data(&gte, 0u, 0x7fff0000u)); /* V0.y */
    assert(psx_gte_write_data(&gte, 1u, 0x00007fffu)); /* V0.z */
    assert(psx_gte_execute(&gte, 0x48006412u)); /* sf=0, lm=1 */
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 32767u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 32767u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* IR2 and IR3 independently clamp one unit above their upper limits. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 2u, 0x00020000u));
    assert(psx_gte_write_control(&gte, 4u, 2u));
    assert(psx_gte_write_data(&gte, 0u, 0x40000000u)); /* V0.y=16384 */
    assert(psx_gte_write_data(&gte, 1u, 16384u)); /* V0.z=16384 */
    assert(psx_gte_execute(&gte, 0x48006412u));
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 32767u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 32767u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 23u)) != 0u); /* IR2 saturation */
    assert((value & (1u << 22u)) != 0u); /* IR3 saturation */
    assert((value & 0x80000000u) != 0u);

    /* Signed IR lower boundary is inclusive at -32768 when lm=0. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0xffff8000u)); /* V0.x=-32768 */
    assert(psx_gte_execute(&gte, 0x48080012u)); /* sf=1, RT, V0, TR, lm=0 */
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 0xffff8000u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* One unit below -32768 clamps and raises IR1 saturation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 5u, 0xffffffffu)); /* TRX=-1 */
    assert(psx_gte_write_data(&gte, 0u, 0xffff8000u));
    assert(psx_gte_execute(&gte, 0x48080012u)); /* sf=1, RT, V0, TR, lm=0 */
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 0xffff8000u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 24u)) != 0u);
    assert((value & 0x80000000u) != 0u);

    /* IR2 and IR3 lower boundaries are inclusive at -32768. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 2u, 0x10000000u)); /* RT[1][1]=4096 */
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u)); /* RT[2][2]=4096 */
    assert(psx_gte_write_data(&gte, 0u, 0x80000000u)); /* V0.y=-32768 */
    assert(psx_gte_write_data(&gte, 1u, 0xffff8000u)); /* V0.z=-32768 */
    assert(psx_gte_execute(&gte, 0x48080012u)); /* sf=1, RT, V0, TR, lm=0 */
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 0xffff8000u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 0xffff8000u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* IR2 and IR3 clamp one unit below -32768 and set their own flags. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 2u, 0x10000000u)); /* RT[1][1]=4096 */
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u)); /* RT[2][2]=4096 */
    assert(psx_gte_write_control(&gte, 6u, 0xffffffffu)); /* TRY=-1 */
    assert(psx_gte_write_control(&gte, 7u, 0xffffffffu)); /* TRZ=-1 */
    assert(psx_gte_write_data(&gte, 0u, 0x80000000u)); /* V0.y=-32768 */
    assert(psx_gte_write_data(&gte, 1u, 0xffff8000u)); /* V0.z=-32768 */
    assert(psx_gte_execute(&gte, 0x48080012u));
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 0xffff8000u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 0xffff8000u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 23u)) != 0u); /* IR2 saturation */
    assert((value & (1u << 22u)) != 0u); /* IR3 saturation */
    assert((value & 0x80000000u) != 0u); /* FLAG summary */

    /* Independent PCSX-Redux reference: RT rotation plus TR translation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0xf0000000u));
    assert(psx_gte_write_control(&gte, 1u, 0x10000000u));
    assert(psx_gte_write_control(&gte, 2u, 0u));
    assert(psx_gte_write_control(&gte, 3u, 0u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 5u, 10u));
    assert(psx_gte_write_control(&gte, 6u, 20u));
    assert(psx_gte_write_control(&gte, 7u, 30u));
    assert(psx_gte_write_data(&gte, 0u, 0x00c80064u)); /* V0=(100,200,300) */
    assert(psx_gte_write_data(&gte, 1u, 300u));
    assert(psx_gte_execute(&gte, 0x48080012u)); /* sf=1, RT, V0, TR */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == (uint32_t)-190);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 120u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 330u);

    /* Independent PCSX-Redux reference: V1 with zero translation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 2u, 0x0028001eu)); /* V1=(30,40,50) */
    assert(psx_gte_write_data(&gte, 3u, 50u));
    assert(psx_gte_execute(&gte, 0x4808e012u)); /* sf=1, RT, V1, zero */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 30u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 40u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 50u);

    /* Independent PCSX-Redux reference: IR vector with zero translation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 9u, 500u));
    assert(psx_gte_write_data(&gte, 10u, 600u));
    assert(psx_gte_write_data(&gte, 11u, 700u));
    assert(psx_gte_execute(&gte, 0x4809e012u)); /* sf=1, RT, IR, zero */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 500u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 600u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 700u);

    /* Independent PCSX-Redux reference: RTPS MAC output with translation. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 5u, 100u)); /* TRX */
    assert(psx_gte_write_control(&gte, 6u, 200u)); /* TRY */
    assert(psx_gte_write_control(&gte, 7u, 300u)); /* TRZ */
    assert(psx_gte_write_data(&gte, 0u, 0x0032000au)); /* V0=(10,50,500) */
    assert(psx_gte_write_data(&gte, 1u, 500u));
    assert(psx_gte_execute(&gte, 0x4a080001u)); /* RTPS, sf=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 110u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 250u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 800u);

    /* Independent PCSX-Redux reference: RTPS sf=0 preserves the
     * unshifted MAC3 while IR3 and SZ3 use their fixed-point paths. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 0x1000u)); /* TRZ */
    assert(psx_gte_write_control(&gte, 26u, 200u)); /* H */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_execute(&gte, 0x4a000001u)); /* RTPS, sf=0 */
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 16777216u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 0x7fffu);
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 4096u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* Independent PCSX-Redux reference: division overflow and screen
     * coordinate saturation are both visible in the RTPS FLAG register. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 26u, 200u)); /* H */
    assert(psx_gte_write_data(&gte, 0u, 0x00007fffu)); /* VX=32767 */
    assert(psx_gte_write_data(&gte, 1u, 100u)); /* VZ=100 */
    assert(psx_gte_execute(&gte, 0x4a080001u)); /* RTPS, sf=1 */
    assert(psx_gte_read_data(&gte, 14u, &value));
    assert((value & 0xffffu) == 0x03ffu); /* SX saturation */
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 17u)) != 0u); /* division overflow */
    assert((value & (1u << 14u)) != 0u); /* SX saturation */
    assert((value & 0x80000000u) != 0u); /* FLAG summary */

    assert(!psx_gte_execute(&gte, 0x4b400007u));
    assert(!psx_gte_execute(0, 0x4b400006u));

    /* RTPT must accumulate flags from earlier vertices, even when
     * the final vertex itself does not overflow the depth FIFO. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 7u, 40000u)); /* TRZ */
    assert(psx_gte_write_control(&gte, 26u, 200u)); /* H */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 32767u)); /* V0 Z */
    assert(psx_gte_write_data(&gte, 2u, 0u));
    assert(psx_gte_write_data(&gte, 3u, 0xffff8000u)); /* V1 Z=-32768 */
    assert(psx_gte_write_data(&gte, 4u, 0u));
    assert(psx_gte_write_data(&gte, 5u, 0xffff8000u)); /* V2 Z=-32768 */
    assert(psx_gte_execute(&gte, 0x4a080030u)); /* RTPT, sf=1 */
    assert(psx_gte_read_data(&gte, 17u, &value) && value == 65535u);
    assert(psx_gte_read_data(&gte, 18u, &value) && value == 7232u);
    assert(psx_gte_read_data(&gte, 19u, &value) && value == 7232u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 18u)) != 0u); /* earlier SZ saturation retained */
    assert((value & 0x80000000u) != 0u); /* summary retained */

    /* RTPT retains IR0 saturation from V0 even though V2 finishes in range.
     * IR0's bit 12 alone must not set the summary bit 31. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 0u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 2u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 4u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 26u, 200u)); /* H */
    assert(psx_gte_write_control(&gte, 27u, 200u)); /* DQA */
    assert(psx_gte_write_control(&gte, 28u, 0u)); /* DQB */
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 101u)); /* V0 Z: IR0 overflows */
    assert(psx_gte_write_data(&gte, 2u, 0u));
    assert(psx_gte_write_data(&gte, 3u, 10000u)); /* V1 Z */
    assert(psx_gte_write_data(&gte, 4u, 0u));
    assert(psx_gte_write_data(&gte, 5u, 32767u)); /* V2 Z: IR0 in range */
    assert(psx_gte_execute(&gte, 0x4a080030u)); /* RTPT, sf=1 */
    assert(psx_gte_read_data(&gte, 8u, &value));
    assert(value <= 4096u);
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 12u)) != 0u); /* V0 IR0 saturation retained */
    assert((value & 0x80000000u) == 0u); /* IR0 excluded from summary */

    /* NCT must retain saturation flags raised by V0 while processing
     * later vertices whose lighting calculations do not saturate. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 10u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 12u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x00007fffu)); /* V0 X=32767 */
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_write_data(&gte, 2u, 0u));
    assert(psx_gte_write_data(&gte, 3u, 0u));
    assert(psx_gte_write_data(&gte, 4u, 0u));
    assert(psx_gte_write_data(&gte, 5u, 0u));
    assert(psx_gte_execute(&gte, 0x4a000420u)); /* NCT, sf=0, lm=1 */
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 24u)) != 0u); /* V0 IR1 saturation retained */
    assert((value & 0x80000000u) != 0u); /* FLAG summary retained */

    /* NCCT must accumulate saturation flags from the first normal
     * across all three complete NCCS lighting/color passes. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 10u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 12u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 6u, 0x00ffffffu)); /* RGBC */
    assert(psx_gte_write_data(&gte, 0u, 0x00007fffu)); /* V0 normal */
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_write_data(&gte, 2u, 0u));
    assert(psx_gte_write_data(&gte, 3u, 0u));
    assert(psx_gte_write_data(&gte, 4u, 0u));
    assert(psx_gte_write_data(&gte, 5u, 0u));
    assert(psx_gte_execute(&gte, 0x4a00043fu)); /* NCCT, sf=0, lm=1 */
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 24u)) != 0u); /* V0 saturation retained */
    assert((value & 0x80000000u) != 0u); /* FLAG summary retained */

    /* NCDT must retain the first normal's saturation flag while
     * executing the depth-cued lighting pipeline for V1 and V2. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 10u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 12u, 0x00007fffu));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 21u, 0u));
    assert(psx_gte_write_control(&gte, 22u, 0u));
    assert(psx_gte_write_control(&gte, 23u, 0u));
    assert(psx_gte_write_data(&gte, 6u, 0x00ffffffu)); /* RGBC */
    assert(psx_gte_write_data(&gte, 0u, 0x00007fffu)); /* V0 normal */
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_write_data(&gte, 2u, 0u));
    assert(psx_gte_write_data(&gte, 3u, 0u));
    assert(psx_gte_write_data(&gte, 4u, 0u));
    assert(psx_gte_write_data(&gte, 5u, 0u));
    assert(psx_gte_execute(&gte, 0x4a000416u)); /* NCDT, sf=0, lm=1 */
    assert(psx_gte_read_control(&gte, 31u, &value));
    assert((value & (1u << 24u)) != 0u); /* V0 saturation retained */
    assert((value & 0x80000000u) != 0u); /* FLAG summary retained */

    /* CC reference vector: identity color matrix and IR=(1,1,1)
     * preserve the RGBC channels, while RGB FIFO entries shift once. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 6u, 0x7f804020u)); /* RGBC */
    assert(psx_gte_write_data(&gte, 9u, 0x1000u));
    assert(psx_gte_write_data(&gte, 10u, 0x1000u));
    assert(psx_gte_write_data(&gte, 11u, 0x1000u));
    assert(psx_gte_write_data(&gte, 20u, 0x11111111u)); /* RGB0 */
    assert(psx_gte_write_data(&gte, 21u, 0x22222222u)); /* RGB1 */
    assert(psx_gte_write_data(&gte, 22u, 0x33333333u)); /* RGB2 */
    assert(psx_gte_execute(&gte, 0x4a08041cu)); /* CC, sf=1, lm=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 0x200u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 0x400u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 0x800u);
    assert(psx_gte_read_data(&gte, 20u, &value) && value == 0x22222222u);
    assert(psx_gte_read_data(&gte, 21u, &value) && value == 0x33333333u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0x7f804020u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* NCDS numeric reference: identity light/color matrices, asymmetric
     * normal and RGBC channels, zero far color and zero depth cue. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 6u, 0x7fc08040u)); /* code, B, G, R */
    assert(psx_gte_write_data(&gte, 0u, 0x08001000u)); /* X=4096, Y=2048 */
    assert(psx_gte_write_data(&gte, 1u, 0x00000400u)); /* Z=1024 */
    assert(psx_gte_write_data(&gte, 8u, 0u));          /* IR0 */
    assert(psx_gte_execute(&gte, 0x4a080413u));        /* NCDS, sf=1, lm=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 1024u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 1024u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 768u);
    assert(psx_gte_read_data(&gte, 9u, &value) && value == 1024u);
    assert(psx_gte_read_data(&gte, 10u, &value) && value == 1024u);
    assert(psx_gte_read_data(&gte, 11u, &value) && value == 768u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0x7f304040u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* NCDT numeric reference: three differently scaled normals must
     * produce three distinct RGB results in FIFO order. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 0u, 0x08001000u)); /* V0=(4096,2048,1024) */
    assert(psx_gte_write_data(&gte, 1u, 1024u));
    assert(psx_gte_write_data(&gte, 2u, 0x04000800u)); /* V1=(2048,1024,512) */
    assert(psx_gte_write_data(&gte, 3u, 512u));
    assert(psx_gte_write_data(&gte, 4u, 0x02000400u)); /* V2=(1024,512,256) */
    assert(psx_gte_write_data(&gte, 5u, 256u));
    assert(psx_gte_write_data(&gte, 6u, 0x7fc08040u));
    assert(psx_gte_write_data(&gte, 8u, 0u));
    assert(psx_gte_execute(&gte, 0x4a080416u)); /* NCDT, sf=1, lm=1 */
    assert(psx_gte_read_data(&gte, 25u, &value) && value == 256u);
    assert(psx_gte_read_data(&gte, 26u, &value) && value == 256u);
    assert(psx_gte_read_data(&gte, 27u, &value) && value == 192u);
    assert(psx_gte_read_data(&gte, 20u, &value) && value == 0x7f304040u);
    assert(psx_gte_read_data(&gte, 21u, &value) && value == 0x7f182020u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0x7f0c1010u);
    assert(psx_gte_read_data(&gte, 0u, &value) && value == 0x08001000u);
    assert(psx_gte_read_data(&gte, 1u, &value) && value == 1024u);
    assert(psx_gte_read_control(&gte, 31u, &value) && value == 0u);

    /* A single color command shifts, rather than replaces, the RGB FIFO. */
    psx_gte_reset(&gte);
    assert(psx_gte_write_control(&gte, 8u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 10u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 12u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 16u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 18u, 0x00001000u));
    assert(psx_gte_write_control(&gte, 20u, 0x00001000u));
    assert(psx_gte_write_data(&gte, 20u, 0x11010203u));
    assert(psx_gte_write_data(&gte, 21u, 0x22040506u));
    assert(psx_gte_write_data(&gte, 22u, 0x33070809u));
    assert(psx_gte_write_data(&gte, 0u, 0u));
    assert(psx_gte_write_data(&gte, 1u, 0u));
    assert(psx_gte_write_data(&gte, 6u, 0x44000000u));
    assert(psx_gte_execute(&gte, 0x4a08001eu)); /* NCS, sf=1 */
    assert(psx_gte_read_data(&gte, 20u, &value) && value == 0x22040506u);
    assert(psx_gte_read_data(&gte, 21u, &value) && value == 0x33070809u);
    assert(psx_gte_read_data(&gte, 22u, &value) && value == 0x44000000u);

    assert(!psx_gte_read_data(0, 0u, &value));
    assert(!psx_gte_write_control(0, 0u, 0u));

    return 0;
}
