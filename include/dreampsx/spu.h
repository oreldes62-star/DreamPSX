#ifndef DREAMPSX_SPU_H
#define DREAMPSX_SPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSX_SPU_BASE_ADDR 0x1f801c00u
#define PSX_SPU_REGISTER_BYTES 0x200u
#define PSX_SPU_RAM_BYTES (512u * 1024u)
#define PSX_SPU_TRANSFER_ADDR_OFFSET 0x1a6u
#define PSX_SPU_TRANSFER_DATA_OFFSET 0x1a8u
#define PSX_SPU_KEY_ON_LOW_OFFSET 0x188u
#define PSX_SPU_KEY_ON_HIGH_OFFSET 0x18au
#define PSX_SPU_KEY_OFF_LOW_OFFSET 0x18cu
#define PSX_SPU_KEY_OFF_HIGH_OFFSET 0x18eu
#define PSX_SPU_CONTROL_OFFSET 0x1aau
#define PSX_SPU_VOICE_COUNT 24u
#define PSX_SPU_VOICE_REGISTER_BYTES 0x10u
#define PSX_SPU_VOICE_CURRENT_VOLUME_OFFSET 0x0cu
#define PSX_SPU_ADPCM_BLOCK_BYTES 16u
#define PSX_SPU_ADPCM_BLOCK_SAMPLES 28u
#define PSX_SPU_STATUS_OFFSET 0x1aeu

typedef struct {
    int32_t history_1;
    int32_t history_2;
} PsxSpuAdpcmState;

typedef struct {
    uint16_t volume_left;
    uint16_t volume_right;
    uint16_t pitch;
    uint16_t start_address;
    uint16_t adsr1;
    uint16_t adsr2;
    uint16_t current_volume;
    uint16_t repeat_address;
    PsxSpuAdpcmState adpcm_state;
    int16_t decoded_samples[PSX_SPU_ADPCM_BLOCK_SAMPLES];
    uint32_t playback_address;
    uint8_t sample_index;
    uint8_t block_flags;
    bool block_valid;
} PsxSpuVoice;

typedef struct {
    PsxSpuVoice voices[PSX_SPU_VOICE_COUNT];
    uint8_t registers[PSX_SPU_REGISTER_BYTES];
    uint8_t ram[PSX_SPU_RAM_BYTES];
    uint32_t transfer_address;
    uint32_t active_voices;
    uint16_t control;
    bool initialized;
} PsxSpu;

void psx_spu_adpcm_reset(PsxSpuAdpcmState *state);
bool psx_spu_adpcm_decode_block(PsxSpuAdpcmState *state,
                                const uint8_t block[PSX_SPU_ADPCM_BLOCK_BYTES],
                                int16_t output[PSX_SPU_ADPCM_BLOCK_SAMPLES],
                                uint8_t *flags);

bool psx_spu_init(PsxSpu *spu);
void psx_spu_reset(PsxSpu *spu);
bool psx_spu_read16(PsxSpu *spu, uint32_t address, uint16_t *value);
bool psx_spu_write16(PsxSpu *spu, uint32_t address, uint16_t value);
uint32_t psx_spu_active_voice_mask(const PsxSpu *spu);
bool psx_spu_voice_next_sample(PsxSpu *spu, unsigned voice_index,
                               int16_t *sample);
bool psx_spu_read8(PsxSpu *spu, uint32_t address, uint8_t *value);
bool psx_spu_write8(PsxSpu *spu, uint32_t address, uint8_t value);

#endif
