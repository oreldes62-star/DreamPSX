#ifndef DREAMPSX_XA_ADPCM_H
#define DREAMPSX_XA_ADPCM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSX_XA_ADPCM_SAMPLES_PER_UNIT 28u

typedef struct {
    int32_t history_1;
    int32_t history_2;
} PsxXaAdpcmState;

/* Decode one XA sound unit's already-unpacked signed sample codes.
 * 4-bit codes must be -8..7; 8-bit codes must be -128..127.
 * Predictor/filter is 0..3 and shift is 0..12 (4-bit) or 0..8 (8-bit).
 * State must be kept per audio channel between sound units. */
bool psx_xa_adpcm_decode_unit(PsxXaAdpcmState *state,
                              const int8_t *codes, size_t code_count,
                              unsigned bits_per_sample,
                              unsigned filter, unsigned shift,
                              int16_t output[PSX_XA_ADPCM_SAMPLES_PER_UNIT]);

void psx_xa_adpcm_reset(PsxXaAdpcmState *state);

#endif
