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
        int32_t residual;
        int32_t predicted;
        int32_t sample;

        if ((bits_per_sample == 4u && (codes[i] < -8 || codes[i] > 7)) ||
            (bits_per_sample == 8u && (codes[i] < -128 || codes[i] > 127)))
            return false;

        residual = (int32_t)codes[i] * (1 << scale);
        residual >>= shift;
        predicted = (h1 * predictor_1[filter] +
                     h2 * predictor_2[filter] + 32) >> 6;
        sample = clamp_sample(residual + predicted);
        h2 = h1;
        h1 = sample;
        output[i] = (int16_t)sample;
    }

    state->history_1 = h1;
    state->history_2 = h2;
    return true;
}

bool psx_xa_adpcm_decode_sound_group(
    const uint8_t group[PSX_XA_ADPCM_GROUP_SIZE],
    unsigned bits_per_sample, bool stereo, PsxXaAdpcmState states[2],
    int16_t left[PSX_XA_ADPCM_MAX_GROUP_SAMPLES],
    int16_t right[PSX_XA_ADPCM_MAX_GROUP_SAMPLES],
    size_t *samples_per_channel)
{
    unsigned units, shift_limit;
    size_t counts[2] = { 0u, 0u };

    if (!group || !states || !left || !right || !samples_per_channel ||
        (bits_per_sample != 4u && bits_per_sample != 8u))
        return false;

    units = bits_per_sample == 4u ? 8u : 4u;
    shift_limit = bits_per_sample == 4u ? 12u : 8u;

    /* Validate every unit header before changing predictor state or output. */
    for (unsigned unit = 0u; unit < units; ++unit) {
        if ((group[4u + unit] & 0x0fu) > shift_limit) return false;
    }

    for (unsigned unit = 0u; unit < units; ++unit) {
        unsigned channel = stereo ? (unit & 1u) : 0u;
        uint8_t parameter = group[4u + unit];
        unsigned shift = parameter & 0x0fu;
        unsigned filter = (parameter >> 4u) & 0x03u;
        int8_t codes[PSX_XA_ADPCM_SAMPLES_PER_UNIT];
        int16_t decoded[PSX_XA_ADPCM_SAMPLES_PER_UNIT];

        for (unsigned sample = 0u; sample < PSX_XA_ADPCM_SAMPLES_PER_UNIT;
             ++sample) {
            uint8_t packed = group[16u + sample * 4u +
                                   (bits_per_sample == 4u ? unit / 2u : unit)];
            if (bits_per_sample == 4u) {
                unsigned nibble = (packed >> ((unit & 1u) * 4u)) & 0x0fu;
                codes[sample] = (int8_t)(nibble >= 8u ?
                                         (int)nibble - 16 : (int)nibble);
            } else {
                codes[sample] = (int8_t)(packed < 0x80u ?
                                         (int)packed : (int)packed - 256);
            }
        }

        if (!psx_xa_adpcm_decode_unit(&states[channel], codes,
                                      PSX_XA_ADPCM_SAMPLES_PER_UNIT,
                                      bits_per_sample, filter, shift, decoded))
            return false;

        for (unsigned sample = 0u; sample < PSX_XA_ADPCM_SAMPLES_PER_UNIT;
             ++sample) {
            size_t index = counts[channel]++;
            if (!stereo || channel == 0u) left[index] = decoded[sample];
            if (!stereo || channel == 1u) right[index] = decoded[sample];
        }
    }

    *samples_per_channel = counts[0];
    return counts[0] == counts[1] || !stereo;
}


bool psx_xa_adpcm_decode_sector(
    const uint8_t *payload, size_t payload_size,
    const PsxDiscXaAudioFormat *format, PsxXaAdpcmState states[2],
    int16_t *left, int16_t *right, size_t output_capacity,
    size_t *samples_per_channel)
{
    PsxXaAdpcmState next_states[2];
    int16_t group_left[PSX_XA_ADPCM_MAX_GROUP_SAMPLES];
    int16_t group_right[PSX_XA_ADPCM_MAX_GROUP_SAMPLES];
    size_t group_samples, required_samples;
    bool stereo;
    unsigned units, shift_limit;

    if (!payload || payload_size < PSX_XA_ADPCM_SECTOR_PAYLOAD_SIZE ||
        !format || !states || !left || !right || !samples_per_channel ||
        (format->channels != 1u && format->channels != 2u) ||
        (format->bits_per_sample != 4u && format->bits_per_sample != 8u))
        return false;

    stereo = format->channels == 2u;
    units = format->bits_per_sample == 4u ? 8u : 4u;
    shift_limit = format->bits_per_sample == 4u ? 12u : 8u;
    group_samples = (size_t)units * PSX_XA_ADPCM_SAMPLES_PER_UNIT /
                    (stereo ? 2u : 1u);
    required_samples = group_samples * PSX_XA_ADPCM_SECTOR_GROUPS;
    if (output_capacity < required_samples) return false;

    /* Validate every group before decoding, so malformed headers cannot
       partially advance the caller's predictor history. */
    for (unsigned group = 0u; group < PSX_XA_ADPCM_SECTOR_GROUPS; ++group) {
        const uint8_t *source =
            payload + (size_t)group * PSX_XA_ADPCM_GROUP_SIZE;
        for (unsigned unit = 0u; unit < units; ++unit) {
            if ((source[4u + unit] & 0x0fu) > shift_limit) return false;
        }
    }

    next_states[0] = states[0];
    next_states[1] = states[1];
    for (unsigned group = 0u; group < PSX_XA_ADPCM_SECTOR_GROUPS; ++group) {
        const uint8_t *source =
            payload + (size_t)group * PSX_XA_ADPCM_GROUP_SIZE;
        size_t decoded = 0u;
        if (!psx_xa_adpcm_decode_sound_group(
                source, format->bits_per_sample, stereo, next_states,
                group_left, group_right, &decoded) ||
            decoded != group_samples)
            return false;
        for (size_t sample = 0u; sample < decoded; ++sample) {
            size_t destination =
                (size_t)group * group_samples + sample;
            left[destination] = group_left[sample];
            right[destination] = group_right[sample];
        }
    }

    states[0] = next_states[0];
    states[1] = next_states[1];
    *samples_per_channel = required_samples;
    return true;
}
