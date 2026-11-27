#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dreampsx/xa_adpcm.h"

int main(void)
{
    PsxXaAdpcmState state;
    int8_t codes[PSX_XA_ADPCM_SAMPLES_PER_UNIT];
    int16_t samples[PSX_XA_ADPCM_SAMPLES_PER_UNIT];

    memset(codes, 0, sizeof(codes));
    psx_xa_adpcm_reset(&state);
    assert(psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 0u, 0u,
                                    samples));
    for (unsigned i = 0u; i < 28u; ++i) assert(samples[i] == 0);

    codes[0] = 7;
    codes[1] = -8;
    assert(psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 0u, 0u,
                                    samples));
    assert(samples[0] == 28672);
    assert(samples[1] == -32768);

    psx_xa_adpcm_reset(&state);
    memset(codes, 0, sizeof(codes));
    codes[0] = 127;
    assert(psx_xa_adpcm_decode_unit(&state, codes, 28u, 8u, 0u, 0u,
                                    samples));
    assert(samples[0] == 32512);
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 27u, 8u, 0u, 0u,
                                     samples));
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 28u, 6u, 0u, 0u,
                                     samples));
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 4u, 0u,
                                     samples));
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 0u, 13u,
                                     samples));
    codes[0] = 8;
    assert(!psx_xa_adpcm_decode_unit(&state, codes, 28u, 4u, 0u, 0u,
                                     samples));
    puts("XA ADPCM tests passed");
    return 0;
}
