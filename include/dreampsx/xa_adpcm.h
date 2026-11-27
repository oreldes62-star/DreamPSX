#ifndef DREAMPSX_XA_ADPCM_H
#define DREAMPSX_XA_ADPCM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSX_XA_ADPCM_SAMPLES_PER_UNIT 28u
#define PSX_XA_ADPCM_GROUP_SIZE 128u
#define PSX_XA_ADPCM_MAX_GROUP_SAMPLES 224u

typedef struct {
    int32_t history_1;
    int32_t history_2;
} PsxXaAdpcmState;

void psx_xa_adpcm_reset(PsxXaAdpcmState *state);

/* Decode one XA sound unit's already-unpacked signed sample codes. */
bool psx_xa_adpcm_decode_unit(PsxXaAdpcmState *state,
                              const int8_t *codes, size_t code_count,
                              unsigned bits_per_sample,
                              unsigned filter, unsigned shift,
                              int16_t output[PSX_XA_ADPCM_SAMPLES_PER_UNIT]);

/* Decode one 128-byte XA sound group. Output is planar PCM: left and right
 * each hold samples_per_channel samples. Mono is copied to both outputs.
 * Keep states[0] and states[1] across groups/sectors for L/R predictor history. */
bool psx_xa_adpcm_decode_sound_group(
    const uint8_t group[PSX_XA_ADPCM_GROUP_SIZE],
    unsigned bits_per_sample, bool stereo, PsxXaAdpcmState states[2],
    int16_t left[PSX_XA_ADPCM_MAX_GROUP_SAMPLES],
    int16_t right[PSX_XA_ADPCM_MAX_GROUP_SAMPLES],
    size_t *samples_per_channel);

#endif
