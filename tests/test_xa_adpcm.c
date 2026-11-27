#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dreampsx/xa_adpcm.h"

int main(void)
{
    PsxXaAdpcmState state;
    PsxXaAdpcmState states[2];
    int8_t codes[PSX_XA_ADPCM_SAMPLES_PER_UNIT];
    int16_t samples[PSX_XA_ADPCM_SAMPLES_PER_UNIT];
    uint8_t group[PSX_XA_ADPCM_GROUP_SIZE];
    uint8_t sector[PSX_XA_ADPCM_SECTOR_PAYLOAD_SIZE];
    int16_t sector_left[PSX_XA_ADPCM_MAX_SECTOR_SAMPLES];
    int16_t sector_right[PSX_XA_ADPCM_MAX_SECTOR_SAMPLES];
    PsxDiscXaAudioFormat format;
    int16_t left[PSX_XA_ADPCM_MAX_GROUP_SAMPLES];
    int16_t right[PSX_XA_ADPCM_MAX_GROUP_SAMPLES];
    size_t count = 0u;

    memset(codes, 0, sizeof(codes));
    psx_xa_adpcm_reset(&state);
    assert(psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 0u, 0u, samples));
    for (unsigned i = 0u; i < 28u; ++i) assert(samples[i] == 0);
    codes[0] = 7; codes[1] = -8;
    assert(psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 0u, 0u, samples));
    assert(samples[0] == 28672 && samples[1] == -32768);

    psx_xa_adpcm_reset(&state);
    memset(codes, 0, sizeof(codes)); codes[0] = 127;
    assert(psx_xa_adpcm_decode_unit(&state, codes, 28u, 8u, 0u, 0u, samples));
    assert(samples[0] == 32512);
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 27u, 8u, 0u, 0u, samples));
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 28u, 6u, 0u, 0u, samples));
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 4u, 0u, samples));
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 0u, 13u, samples));
    codes[0] = 8;
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 0u, 0u, samples));

    memset(group, 0, sizeof(group));
    psx_xa_adpcm_reset(&states[0]); psx_xa_adpcm_reset(&states[1]);
    assert(psx_xa_adpcm_decode_sound_group(group, 4u, false, states,
                                            left, right, &count));
    assert(count == 224u);
    for (size_t i = 0u; i < count; ++i)
        assert(left[i] == 0 && right[i] == 0);

    /* Each 4-bit unit occupies one nibble lane; unit 0 is low nibble,
       unit 1 high nibble, and so on. */
    memset(group, 0, sizeof(group));
    group[16] = 0x71u;
    psx_xa_adpcm_reset(&states[0]); psx_xa_adpcm_reset(&states[1]);
    assert(psx_xa_adpcm_decode_sound_group(group, 4u, true, states,
                                            left, right, &count));
    assert(count == 112u);
    assert(left[0] == 4096); /* unit 0, code +1 */
    assert(right[0] == 28672); /* unit 1, code +7 */

    /* 8-bit mono has four units x 28 samples; data bytes are lane-interleaved. */
    memset(group, 0, sizeof(group));
    group[16] = 0x7fu;
    psx_xa_adpcm_reset(&states[0]); psx_xa_adpcm_reset(&states[1]);
    assert(psx_xa_adpcm_decode_sound_group(group, 8u, false, states,
                                            left, right, &count));
    assert(count == 112u);
    assert(left[0] == 32512 && right[0] == left[0]);
    group[4] = 0x0du;
    assert(!psx_xa_adpcm_decode_sound_group(group, 4u, false, states,
                                             left, right, &count));
    /* Decode all 18 groups of a 2324-byte sector, preserving channel state. */
    memset(sector, 0, sizeof(sector));
    sector[16] = 0x71u;
    format.sample_rate_hz = 37800u;
    format.channels = 1u;
    format.bits_per_sample = 4u;
    format.emphasis = false;
    psx_xa_adpcm_reset(&states[0]); psx_xa_adpcm_reset(&states[1]);
    assert(psx_xa_adpcm_decode_sector(
        sector, sizeof(sector), &format, states, sector_left, sector_right,
        PSX_XA_ADPCM_MAX_SECTOR_SAMPLES, &count));
    assert(count == 4032u);
    assert(sector_left[0] == 4096);
    assert(sector_left[28] == 28672);
    assert(sector_right[28] == sector_left[28]);

    /* Stereo 4-bit sectors produce 2016 samples per channel. */
    format.channels = 2u;
    psx_xa_adpcm_reset(&states[0]); psx_xa_adpcm_reset(&states[1]);
    assert(psx_xa_adpcm_decode_sector(
        sector, sizeof(sector), &format, states, sector_left, sector_right,
        PSX_XA_ADPCM_MAX_SECTOR_SAMPLES, &count));
    assert(count == 2016u);
    assert(sector_left[0] == 4096 && sector_right[0] == 28672);

    /* 8-bit sectors produce 2016 mono samples. */
    memset(sector, 0, sizeof(sector));
    sector[16] = 0x7fu;
    format.channels = 1u;
    format.bits_per_sample = 8u;
    psx_xa_adpcm_reset(&states[0]); psx_xa_adpcm_reset(&states[1]);
    assert(psx_xa_adpcm_decode_sector(
        sector, sizeof(sector), &format, states, sector_left, sector_right,
        PSX_XA_ADPCM_MAX_SECTOR_SAMPLES, &count));
    assert(count == 2016u && sector_left[0] == 32512);

    /* Short input, insufficient output capacity and bad late headers fail
       without advancing predictor history or publishing a sample count. */
    {
        PsxXaAdpcmState before[2];
        size_t unchanged_count = 123u;
        before[0] = states[0]; before[1] = states[1];
        assert(!psx_xa_adpcm_decode_sector(
            sector, sizeof(sector) - 1u, &format, states, sector_left,
            sector_right, PSX_XA_ADPCM_MAX_SECTOR_SAMPLES, &unchanged_count));
        assert(!psx_xa_adpcm_decode_sector(
            sector, sizeof(sector), &format, states, sector_left,
            sector_right, 10u, &unchanged_count));
        sector[17u * PSX_XA_ADPCM_GROUP_SIZE + 4u] = 0x0du;
        assert(!psx_xa_adpcm_decode_sector(
            sector, sizeof(sector), &format, states, sector_left,
            sector_right, PSX_XA_ADPCM_MAX_SECTOR_SAMPLES, &unchanged_count));
        assert(states[0].history_1 == before[0].history_1);
        assert(states[0].history_2 == before[0].history_2);
        assert(states[1].history_1 == before[1].history_1);
        assert(states[1].history_2 == before[1].history_2);
        assert(unchanged_count == 123u);
    }

    puts("XA ADPCM tests passed");
    return 0;
}
