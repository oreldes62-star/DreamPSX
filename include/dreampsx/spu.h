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
#define PSX_SPU_REVERB_REGISTER_START_OFFSET 0x1c0u
#define PSX_SPU_REVERB_REGISTER_END_OFFSET 0x1feu
#define PSX_SPU_REVERB_OUTPUT_VOLUME_LEFT_OFFSET 0x084u
#define PSX_SPU_REVERB_OUTPUT_VOLUME_RIGHT_OFFSET 0x086u
#define PSX_SPU_REVERB_ENABLE_LOW_OFFSET 0x198u
#define PSX_SPU_REVERB_ENABLE_HIGH_OFFSET 0x19au
#define PSX_SPU_REVERB_WORK_AREA_START_OFFSET 0x1a2u
#define PSX_SPU_REVERB_MASTER_ENABLE 0x0080u
#define PSX_SPU_REVERB_BUFFER_END_ADDRESS 0x7fffeu
#define PSX_SPU_REVERB_ADDRESS_UNIT_BYTES 8u
#define PSX_SPU_REVERB_CONFIG_DAPF1_OFFSET 0x1c0u
#define PSX_SPU_REVERB_CONFIG_DAPF2_OFFSET 0x1c2u
#define PSX_SPU_REVERB_CONFIG_VIIR_OFFSET 0x1c4u
#define PSX_SPU_REVERB_CONFIG_VCOMB1_OFFSET 0x1c6u
#define PSX_SPU_REVERB_CONFIG_VCOMB2_OFFSET 0x1c8u
#define PSX_SPU_REVERB_CONFIG_VCOMB3_OFFSET 0x1cau
#define PSX_SPU_REVERB_CONFIG_VCOMB4_OFFSET 0x1ccu
#define PSX_SPU_REVERB_CONFIG_VWALL_OFFSET 0x1ceu
#define PSX_SPU_REVERB_CONFIG_VAPF1_OFFSET 0x1d0u
#define PSX_SPU_REVERB_CONFIG_VAPF2_OFFSET 0x1d2u
#define PSX_SPU_REVERB_CONFIG_MLSAME_OFFSET 0x1d4u
#define PSX_SPU_REVERB_CONFIG_MRSAME_OFFSET 0x1d6u
#define PSX_SPU_REVERB_CONFIG_MLCOMB1_OFFSET 0x1d8u
#define PSX_SPU_REVERB_CONFIG_MRCOMB1_OFFSET 0x1dau
#define PSX_SPU_REVERB_CONFIG_MLCOMB2_OFFSET 0x1dcu
#define PSX_SPU_REVERB_CONFIG_MRCOMB2_OFFSET 0x1deu
#define PSX_SPU_REVERB_CONFIG_DLSAME_OFFSET 0x1e0u
#define PSX_SPU_REVERB_CONFIG_DRSAME_OFFSET 0x1e2u
#define PSX_SPU_REVERB_CONFIG_MLDIFF_OFFSET 0x1e4u
#define PSX_SPU_REVERB_CONFIG_MRDIFF_OFFSET 0x1e6u
#define PSX_SPU_REVERB_CONFIG_MLCOMB3_OFFSET 0x1e8u
#define PSX_SPU_REVERB_CONFIG_MRCOMB3_OFFSET 0x1eau
#define PSX_SPU_REVERB_CONFIG_MLCOMB4_OFFSET 0x1ecu
#define PSX_SPU_REVERB_CONFIG_MRCOMB4_OFFSET 0x1eeu
#define PSX_SPU_REVERB_CONFIG_DLDIFF_OFFSET 0x1f0u
#define PSX_SPU_REVERB_CONFIG_DRDIFF_OFFSET 0x1f2u
#define PSX_SPU_REVERB_CONFIG_MLAPF1_OFFSET 0x1f4u
#define PSX_SPU_REVERB_CONFIG_MRAPF1_OFFSET 0x1f6u
#define PSX_SPU_REVERB_CONFIG_MLAPF2_OFFSET 0x1f8u
#define PSX_SPU_REVERB_CONFIG_MRAPF2_OFFSET 0x1fau
#define PSX_SPU_REVERB_CONFIG_VLIN_OFFSET 0x1fcu
#define PSX_SPU_REVERB_CONFIG_VRIN_OFFSET 0x1feu
#define PSX_SPU_ENVELOPE_MAX 0x7fffu

typedef struct {
    int32_t history_1;
    int32_t history_2;
} PsxSpuAdpcmState;

typedef enum {
    PSX_SPU_ADSR_OFF = 0,
    PSX_SPU_ADSR_ATTACK,
    PSX_SPU_ADSR_DECAY,
    PSX_SPU_ADSR_SUSTAIN,
    PSX_SPU_ADSR_RELEASE
} PsxSpuAdsrPhase;

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
    uint16_t pitch_phase;
    int16_t current_sample;
    bool block_valid;
    bool current_sample_valid;
    uint16_t envelope_level;
    uint16_t envelope_counter;
    PsxSpuAdsrPhase envelope_phase;
} PsxSpuVoice;

typedef struct {
    PsxSpuVoice voices[PSX_SPU_VOICE_COUNT];
    uint8_t registers[PSX_SPU_REGISTER_BYTES];
    uint8_t ram[PSX_SPU_RAM_BYTES];
    uint32_t transfer_address;
    uint32_t reverb_buffer_address;
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
bool psx_spu_voice_render_sample(PsxSpu *spu, unsigned voice_index,
                                 int16_t *sample);
bool psx_spu_voice_envelope_tick(PsxSpu *spu, unsigned voice_index,
                                 uint16_t *level);
bool psx_spu_mix_sample(PsxSpu *spu, int16_t *left, int16_t *right);
int16_t psx_spu_reverb_saturate(int32_t value);
int32_t psx_spu_reverb_multiply_q15(int16_t lhs, int16_t rhs);
bool psx_spu_reverb_process_reflections(PsxSpu *spu, int16_t left_input,
                                       int16_t right_input);
bool psx_spu_reverb_process_comb(const PsxSpu *spu,
                                   int16_t *left_output,
                                   int16_t *right_output);
uint32_t psx_spu_reverb_resolve_address(const PsxSpu *spu,
                                        uint16_t relative_address);
void psx_spu_reverb_advance(PsxSpu *spu);
bool psx_spu_reverb_read16(const PsxSpu *spu, uint16_t relative_address,
                           int16_t *value);
bool psx_spu_reverb_write16(PsxSpu *spu, uint16_t relative_address,
                            int16_t value);
bool psx_spu_mix_block(PsxSpu *spu, int16_t *interleaved_stereo,
                       size_t frame_capacity, size_t *frames_written);
bool psx_spu_read8(PsxSpu *spu, uint32_t address, uint8_t *value);
bool psx_spu_write8(PsxSpu *spu, uint32_t address, uint8_t value);

#endif
