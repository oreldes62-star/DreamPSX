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
    puts("XA ADPCM tests passed");
    return 0;
}
