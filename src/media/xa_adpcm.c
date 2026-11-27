#include "dreampsx/xa_adpcm.h"

#include <limits.h>

static const int16_t predictor_1[4] = { 0, 60, 115, 98 };
static const int16_t predictor_2[4] = { 0, 0, -52, -55 };

static int16_t clamp_sample(int32_t sample)
{
    if (sample > INT16_MAX) return INT16_MAX;
    if (sample < INT16_MIN) return INT16_MIN;
    return (int16_t)sample;
}

void psx_xa_adpcm_reset(PsxXaAdpcmState *state)
{
    if (!state) return;
    state->history_1 = 0;
    state->history_2 = 0;
}

bool psx_xa_adpcm_decode_unit(PsxXaAdpcmState *state,
                              const int8_t *codes, size_t code_count,
                              unsigned bits_per_sample,
                              unsigned filter, unsigned shift,
                              int16_t output[PSX_XA_ADPCM_SAMPLES_PER_UNIT])
{
    int32_t h1, h2;
    unsigned scale;

    if (!state || !codes || !output ||
        code_count != PSX_XA_ADPCM_SAMPLES_PER_UNIT ||
        filter >= 4u ||
        (bits_per_sample != 4u && bits_per_sample != 8u) ||
        shift > (bits_per_sample == 4u ? 12u : 8u))
        return false;

    h1 = state->history_1;
    h2 = state->history_2;
    scale = bits_per_sample == 4u ? 12u : 8u;

    for (size_t i = 0u; i < code_count; ++i) {
        int32_t residual = (int32_t)codes[i] * (1 << scale);
        int32_t predicted;
        int32_t sample;

        if ((bits_per_sample == 4u && (codes[i] < -8 || codes[i] > 7)) ||
            (bits_per_sample == 8u && (codes[i] < -128 || codes[i] > 127)))
            return false;

        residual >>= shift;
        predicted = (h1 * predictor_1[filter] +
                     h2 * predictor_2[filter] + 32) >> 6;
        sample = residual + predicted;
        sample = clamp_sample(sample);
        h2 = h1;
        h1 = sample;
        output[i] = (int16_t)sample;
    }

    state->history_1 = h1;
    state->history_2 = h2;
    return true;
}
