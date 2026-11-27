#include "dreampsx/spu.h"

#include <limits.h>
#include <string.h>

static const int16_t predictor_1[5] = { 0, 60, 115, 98, 122 };
static const int16_t predictor_2[5] = { 0, 0, -52, -55, -60 };

static int16_t clamp_spu_sample(int32_t sample)
{
    if (sample > INT16_MAX) return INT16_MAX;
    if (sample < INT16_MIN) return INT16_MIN;
    return (int16_t)sample;
}

void psx_spu_adpcm_reset(PsxSpuAdpcmState *state)
{
    if (!state) return;
    state->history_1 = 0;
    state->history_2 = 0;
}

bool psx_spu_adpcm_decode_block(
    PsxSpuAdpcmState *state,
    const uint8_t block[PSX_SPU_ADPCM_BLOCK_BYTES],
    int16_t output[PSX_SPU_ADPCM_BLOCK_SAMPLES],
    uint8_t *flags)
{
    PsxSpuAdpcmState next;
    unsigned shift, filter;

    if (!state || !block || !output) return false;
    shift = block[0] & 0x0fu;
    filter = block[0] >> 4u;
    if (shift > 12u || filter >= 5u) return false;

    next = *state;
    for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_SAMPLES; ++i) {
        const uint8_t packed = block[2u + i / 2u];
        const unsigned nibble = (packed >> ((i & 1u) * 4u)) & 0x0fu;
        const int32_t code = nibble < 8u ? (int32_t)nibble :
                             (int32_t)nibble - 16;
        const int32_t residual = code * (1 << (12u - shift));
        const int32_t predicted =
            (next.history_1 * predictor_1[filter] +
             next.history_2 * predictor_2[filter] + 32) >> 6;
        const int16_t sample = clamp_spu_sample(residual + predicted);

        output[i] = sample;
        next.history_2 = next.history_1;
        next.history_1 = sample;
    }

    *state = next;
    if (flags) *flags = block[1];
    return true;
}
