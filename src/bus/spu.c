#include "dreampsx/spu.h"

#include <limits.h>
#include <string.h>

static bool register_offset(uint32_t address, uint32_t *offset)
{
    if (address < PSX_SPU_BASE_ADDR ||
        address >= PSX_SPU_BASE_ADDR + PSX_SPU_REGISTER_BYTES)
        return false;
    *offset = address - PSX_SPU_BASE_ADDR;
    return true;
}

static uint16_t load_le16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] |
                      ((uint16_t)bytes[1] << 8u));
}

static void store_le16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8u);
}

bool psx_spu_init(PsxSpu *spu)
{
    if (!spu) return false;
    memset(spu, 0, sizeof(*spu));
    spu->initialized = true;
    return true;
}

void psx_spu_reset(PsxSpu *spu)
{
    if (!spu || !spu->initialized) return;
    memset(spu->registers, 0, sizeof(spu->registers));
    memset(spu->voices, 0, sizeof(spu->voices));
    memset(spu->ram, 0, sizeof(spu->ram));
    spu->transfer_address = 0u;
    spu->reverb_buffer_address = 0u;
    spu->active_voices = 0u;
    spu->control = 0u;
}

bool psx_spu_read16(PsxSpu *spu, uint32_t address, uint16_t *value)
{
    uint32_t offset;
    if (!spu || !spu->initialized || !value ||
        (address & 1u) != 0u || !register_offset(address, &offset))
        return false;

    if (offset == PSX_SPU_KEY_ON_LOW_OFFSET ||
        offset == PSX_SPU_KEY_ON_HIGH_OFFSET ||
        offset == PSX_SPU_KEY_OFF_LOW_OFFSET ||
        offset == PSX_SPU_KEY_OFF_HIGH_OFFSET) {
        *value = 0u; /* Key registers are write-only. */
        return true;
    }
    if (offset == PSX_SPU_CONTROL_OFFSET) {
        *value = spu->control;
        return true;
    }
    if (offset < PSX_SPU_VOICE_COUNT * PSX_SPU_VOICE_REGISTER_BYTES &&
        (offset % PSX_SPU_VOICE_REGISTER_BYTES) ==
            PSX_SPU_VOICE_CURRENT_VOLUME_OFFSET) {
        *value = spu->voices[offset / PSX_SPU_VOICE_REGISTER_BYTES].envelope_level;
        return true;
    }
    if (offset == PSX_SPU_TRANSFER_DATA_OFFSET) {
        const uint32_t ram_address = spu->transfer_address &
                                     (PSX_SPU_RAM_BYTES - 1u);
        *value = load_le16(&spu->ram[ram_address]);
        spu->transfer_address = (ram_address + 2u) &
                                (PSX_SPU_RAM_BYTES - 1u);
        return true;
    }
    *value = load_le16(&spu->registers[offset]);
    return true;
}

bool psx_spu_write16(PsxSpu *spu, uint32_t address, uint16_t value)
{
    uint32_t offset;
    if (!spu || !spu->initialized || (address & 1u) != 0u ||
        !register_offset(address, &offset))
        return false;

    if (offset == PSX_SPU_STATUS_OFFSET)
        return true; /* Read-only status register in this initial model. */

    if (offset == PSX_SPU_KEY_ON_LOW_OFFSET ||
        offset == PSX_SPU_KEY_ON_HIGH_OFFSET) {
        const uint32_t mask = offset == PSX_SPU_KEY_ON_LOW_OFFSET ?
            (uint32_t)value : ((uint32_t)(value & 0x00ffu) << 16u);
        for (unsigned i = 0u; i < PSX_SPU_VOICE_COUNT; ++i) {
            if ((mask & (1u << i)) == 0u) continue;
            PsxSpuVoice *voice = &spu->voices[i];
            psx_spu_adpcm_reset(&voice->adpcm_state);
            voice->playback_address =
                ((uint32_t)voice->start_address * 8u) &
                (PSX_SPU_RAM_BYTES - 1u);
            voice->sample_index = 0u;
            voice->block_flags = 0u;
            voice->pitch_phase = 0u;
            voice->current_sample = 0;
            voice->current_sample_valid = false;
            voice->envelope_level = 0u;
            voice->envelope_counter = 0u;
            voice->envelope_phase = PSX_SPU_ADSR_ATTACK;
            voice->current_volume = 0u;
            voice->block_valid = false;
        }
        spu->active_voices |= mask;
        return true;
    }
    if (offset == PSX_SPU_KEY_OFF_LOW_OFFSET ||
        offset == PSX_SPU_KEY_OFF_HIGH_OFFSET) {
        const uint32_t mask = offset == PSX_SPU_KEY_OFF_LOW_OFFSET ?
            (uint32_t)value : ((uint32_t)(value & 0x00ffu) << 16u);
        for (unsigned i = 0u; i < PSX_SPU_VOICE_COUNT; ++i) {
            if ((mask & (1u << i)) == 0u) continue;
            PsxSpuVoice *voice = &spu->voices[i];
            voice->block_valid = false;
            voice->current_sample_valid = false;
            voice->envelope_counter = 0u;
            if (voice->envelope_level == 0u) {
                voice->envelope_phase = PSX_SPU_ADSR_OFF;
                spu->active_voices &= ~(1u << i);
            } else {
                voice->envelope_phase = PSX_SPU_ADSR_RELEASE;
            }
        }
        return true;
    }
    if (offset == PSX_SPU_REVERB_WORK_AREA_START_OFFSET) {
        store_le16(&spu->registers[offset], value);
        spu->reverb_buffer_address =
            ((uint32_t)value * PSX_SPU_REVERB_ADDRESS_UNIT_BYTES) &
            PSX_SPU_REVERB_BUFFER_END_ADDRESS;
        return true;
    }
    if (offset == PSX_SPU_CONTROL_OFFSET) {
        spu->control = value;
        store_le16(&spu->registers[offset], value);
        return true;
    }
    if (offset < PSX_SPU_VOICE_COUNT * PSX_SPU_VOICE_REGISTER_BYTES) {
        const unsigned voice_index = offset / PSX_SPU_VOICE_REGISTER_BYTES;
        const unsigned voice_offset = offset % PSX_SPU_VOICE_REGISTER_BYTES;
        PsxSpuVoice *voice = &spu->voices[voice_index];
        switch (voice_offset) {
        case 0x00u: voice->volume_left = value; break;
        case 0x02u: voice->volume_right = value; break;
        case 0x04u: voice->pitch = (uint16_t)(value & 0x3fffu); break;
        case 0x06u: voice->start_address = value; break;
        case 0x08u: voice->adsr1 = value; break;
        case 0x0au: voice->adsr2 = value; break;
        case 0x0cu: return true; /* Current volume is read-only. */
        case 0x0eu: voice->repeat_address = value; break;
        default: return false;
        }
        store_le16(&spu->registers[offset], value);
        return true;
    }

    if (offset == PSX_SPU_TRANSFER_ADDR_OFFSET) {
        store_le16(&spu->registers[offset], value);
        spu->transfer_address =
            ((uint32_t)value * 8u) & (PSX_SPU_RAM_BYTES - 1u);
        return true;
    }
    if (offset == PSX_SPU_TRANSFER_DATA_OFFSET) {
        const uint32_t ram_address = spu->transfer_address &
                                     (PSX_SPU_RAM_BYTES - 1u);
        store_le16(&spu->ram[ram_address], value);
        spu->transfer_address = (ram_address + 2u) &
                                (PSX_SPU_RAM_BYTES - 1u);
        return true;
    }

    store_le16(&spu->registers[offset], value);
    return true;
}

bool psx_spu_voice_next_sample(PsxSpu *spu, unsigned voice_index,
                               int16_t *sample)
{
    PsxSpuVoice *voice;
    uint8_t block[PSX_SPU_ADPCM_BLOCK_BYTES];
    if (!spu || !spu->initialized || !sample ||
        voice_index >= PSX_SPU_VOICE_COUNT ||
        (spu->active_voices & (1u << voice_index)) == 0u)
        return false;

    voice = &spu->voices[voice_index];
    if (!voice->block_valid ||
        voice->sample_index >= PSX_SPU_ADPCM_BLOCK_SAMPLES) {
        const uint32_t block_address = voice->playback_address;
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; ++i)
            block[i] = spu->ram[(block_address + i) &
                                (PSX_SPU_RAM_BYTES - 1u)];
        if (!psx_spu_adpcm_decode_block(&voice->adpcm_state, block,
                                        voice->decoded_samples,
                                        &voice->block_flags)) {
            spu->active_voices &= ~(1u << voice_index);
            voice->block_valid = false;
            return false;
        }
        /* ADPCM flag bit 1 marks a loop-start block. The SPU captures
         * its address in the voice repeat-address register (8-byte units). */
        if ((voice->block_flags & 0x02u) != 0u) {
            voice->repeat_address = (uint16_t)(block_address / 8u);
            store_le16(&spu->registers[
                voice_index * PSX_SPU_VOICE_REGISTER_BYTES + 0x0eu],
                voice->repeat_address);
        }
        voice->playback_address = (block_address +
            PSX_SPU_ADPCM_BLOCK_BYTES) & (PSX_SPU_RAM_BYTES - 1u);
        voice->sample_index = 0u;
        voice->block_valid = true;
    }

    *sample = voice->decoded_samples[voice->sample_index++];
    if (voice->sample_index == PSX_SPU_ADPCM_BLOCK_SAMPLES &&
        (voice->block_flags & 0x01u) != 0u) {
        voice->block_valid = false;
        if ((voice->block_flags & 0x04u) != 0u) {
            /* End + repeat: restart from the programmed/captured loop
             * address on the next source-sample request. */
            voice->playback_address =
                ((uint32_t)voice->repeat_address * 8u) &
                (PSX_SPU_RAM_BYTES - 1u);
            voice->sample_index = 0u;
        } else {
            spu->active_voices &= ~(1u << voice_index);
        }
    }
    return true;
}

static void adsr_rate_parameters(const PsxSpuVoice *voice,
                                  PsxSpuAdsrPhase phase,
                                  unsigned *shift, unsigned *step,
                                  bool *exponential, bool *decreasing)
{
    switch (phase) {
    case PSX_SPU_ADSR_ATTACK: {
        const unsigned rate = (voice->adsr1 >> 8u) & 0x7fu;
        *shift = rate >> 2u;
        *step = rate & 3u;
        *exponential = (voice->adsr1 & 0x8000u) != 0u;
        *decreasing = false;
        break;
    }
    case PSX_SPU_ADSR_DECAY:
        *shift = (voice->adsr1 >> 4u) & 0x0fu;
        *step = 0u; /* Fixed -8 decrement. */
        *exponential = true;
        *decreasing = true;
        break;
    case PSX_SPU_ADSR_SUSTAIN: {
        const unsigned rate = (voice->adsr2 >> 6u) & 0x7fu;
        *shift = rate >> 2u;
        *step = rate & 3u;
        *exponential = (voice->adsr2 & 0x8000u) != 0u;
        *decreasing = (voice->adsr2 & 0x4000u) != 0u;
        break;
    }
    case PSX_SPU_ADSR_RELEASE:
        *shift = voice->adsr2 & 0x1fu;
        *step = 0u; /* Fixed -8 decrement. */
        *exponential = (voice->adsr2 & 0x0020u) != 0u;
        *decreasing = true;
        break;
    default:
        *shift = 31u;
        *step = 3u;
        *exponential = false;
        *decreasing = true;
        break;
    }
}

bool psx_spu_voice_envelope_tick(PsxSpu *spu, unsigned voice_index,
                                 uint16_t *level)
{
    PsxSpuVoice *voice;
    unsigned shift, step_value;
    bool exponential, decreasing;
    int32_t step;
    uint32_t counter_increment;
    uint32_t counter;

    if (!spu || !spu->initialized || voice_index >= PSX_SPU_VOICE_COUNT)
        return false;
    voice = &spu->voices[voice_index];
    if ((spu->active_voices & (1u << voice_index)) == 0u ||
        voice->envelope_phase == PSX_SPU_ADSR_OFF) {
        if (level) *level = voice->envelope_level;
        return false;
    }

    if (voice->envelope_phase == PSX_SPU_ADSR_ATTACK &&
        voice->envelope_level >= PSX_SPU_ENVELOPE_MAX)
        voice->envelope_phase = PSX_SPU_ADSR_DECAY;
    if (voice->envelope_phase == PSX_SPU_ADSR_DECAY &&
        voice->envelope_level <=
            (uint16_t)(((voice->adsr1 & 0x0fu) + 1u) << 11u))
        voice->envelope_phase = PSX_SPU_ADSR_SUSTAIN;
    if (voice->envelope_phase == PSX_SPU_ADSR_RELEASE &&
        voice->envelope_level == 0u) {
        voice->envelope_phase = PSX_SPU_ADSR_OFF;
        spu->active_voices &= ~(1u << voice_index);
        if (level) *level = 0u;
        return false;
    }

    adsr_rate_parameters(voice, voice->envelope_phase, &shift,
                         &step_value, &exponential, &decreasing);
    step = (int32_t)(7u - step_value);
    if (decreasing) step = ~step;
    step *= (int32_t)(1u << ((shift < 11u) ? (11u - shift) : 0u));
    counter_increment = 0x8000u >> ((shift > 11u) ? (shift - 11u) : 0u);

    if (exponential && !decreasing &&
        voice->envelope_level > 0x6000u) {
        if (shift < 10u) step >>= 2;
        else if (shift >= 11u) counter_increment >>= 2u;
        else {
            step >>= 1;
            counter_increment >>= 1u;
        }
    } else if (exponential && decreasing) {
        step = (step * (int32_t)voice->envelope_level) / 0x8000;
    }

    if ((step_value | (shift << 2u)) != 0x7fu &&
        counter_increment == 0u)
        counter_increment = 1u;
    counter = ((uint32_t)voice->envelope_counter +
               counter_increment) & 0xffffu;
    voice->envelope_counter = (uint16_t)counter;

    if ((counter & 0x8000u) != 0u) {
        int32_t next = (int32_t)voice->envelope_level + step;
        if (!decreasing && next > (int32_t)PSX_SPU_ENVELOPE_MAX)
            next = (int32_t)PSX_SPU_ENVELOPE_MAX;
        if (decreasing && next < 0) next = 0;
        if (voice->envelope_phase == PSX_SPU_ADSR_DECAY) {
            const int32_t sustain =
                (int32_t)(((voice->adsr1 & 0x0fu) + 1u) << 11u);
            if (next < sustain) next = sustain;
        }
        voice->envelope_level = (uint16_t)next;
    }

    if (voice->envelope_phase == PSX_SPU_ADSR_ATTACK &&
        voice->envelope_level == PSX_SPU_ENVELOPE_MAX)
        voice->envelope_phase = PSX_SPU_ADSR_DECAY;
    if (voice->envelope_phase == PSX_SPU_ADSR_DECAY &&
        voice->envelope_level <=
            (uint16_t)(((voice->adsr1 & 0x0fu) + 1u) << 11u))
        voice->envelope_phase = PSX_SPU_ADSR_SUSTAIN;
    if (voice->envelope_phase == PSX_SPU_ADSR_RELEASE &&
        voice->envelope_level == 0u) {
        voice->envelope_phase = PSX_SPU_ADSR_OFF;
        spu->active_voices &= ~(1u << voice_index);
    }

    voice->current_volume = voice->envelope_level;
    if (level) *level = voice->envelope_level;
    return voice->envelope_phase != PSX_SPU_ADSR_OFF;
}

bool psx_spu_voice_render_sample(PsxSpu *spu, unsigned voice_index,
                                 int16_t *sample)
{
    PsxSpuVoice *voice;
    uint32_t phase;

    if (!spu || !spu->initialized || !sample ||
        voice_index >= PSX_SPU_VOICE_COUNT ||
        (spu->active_voices & (1u << voice_index)) == 0u)
        return false;

    voice = &spu->voices[voice_index];
    if (!voice->current_sample_valid) {
        if (!psx_spu_voice_next_sample(spu, voice_index,
                                       &voice->current_sample))
            return false;
        voice->current_sample_valid = true;
        *sample = voice->current_sample;
        return true;
    }

    phase = (uint32_t)voice->pitch_phase + voice->pitch;
    while (phase >= 0x1000u) {
        phase -= 0x1000u;
        if (!psx_spu_voice_next_sample(spu, voice_index,
                                       &voice->current_sample)) {
            voice->current_sample_valid = false;
            return false;
        }
    }
    voice->pitch_phase = (uint16_t)phase;
    *sample = voice->current_sample;
    return true;
}


static int16_t clamp_audio_sample(int64_t sample)
{
    if (sample > 32767) return 32767;
    if (sample < -32768) return -32768;
    return (int16_t)sample;
}

bool psx_spu_mix_sample(PsxSpu *spu, int16_t *left, int16_t *right)
{
    int64_t mix_left = 0;
    int64_t mix_right = 0;
    bool produced = false;

    if (!spu || !spu->initialized || !left || !right)
        return false;

    for (unsigned i = 0u; i < PSX_SPU_VOICE_COUNT; ++i) {
        PsxSpuVoice *voice = &spu->voices[i];
        int16_t sample;
        uint16_t envelope;
        int32_t volume_left;
        int32_t volume_right;
        int64_t enveloped;

        if ((spu->active_voices & (1u << i)) == 0u)
            continue;
        if (!psx_spu_voice_envelope_tick(spu, i, &envelope))
            continue;
        if (!psx_spu_voice_render_sample(spu, i, &sample))
            continue;

        /* Fixed-volume mode: signed 15-bit register value, scaled by ENVX.
         * Sweep-mode registers are intentionally not implemented yet. */
        volume_left = (int16_t)voice->volume_left;
        volume_right = (int16_t)voice->volume_right;
        enveloped = ((int64_t)sample * envelope) / PSX_SPU_ENVELOPE_MAX;
        mix_left += (enveloped * volume_left) / 32767;
        mix_right += (enveloped * volume_right) / 32767;
        produced = true;
    }

    *left = clamp_audio_sample(mix_left);
    *right = clamp_audio_sample(mix_right);
    return produced;
}



int16_t psx_spu_reverb_saturate(int32_t value)
{
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

int32_t psx_spu_reverb_multiply_q15(int16_t lhs, int16_t rhs)
{
    const int32_t product = (int32_t)lhs * (int32_t)rhs;
    /* Keep the scaled product wide: comb-filter terms are accumulated
     * before the final 16-bit work-RAM saturation. */
    return product / 0x8000;
}

uint32_t psx_spu_reverb_resolve_address(const PsxSpu *spu,
                                        uint16_t relative_address)
{
    uint32_t base;
    uint32_t address;
    if (!spu || !spu->initialized)
        return 0u;
    base = ((uint32_t)load_le16(&spu->registers[
        PSX_SPU_REVERB_WORK_AREA_START_OFFSET]) *
        PSX_SPU_REVERB_ADDRESS_UNIT_BYTES) &
        PSX_SPU_REVERB_BUFFER_END_ADDRESS;
    address = (spu->reverb_buffer_address +
        (uint32_t)relative_address * PSX_SPU_REVERB_ADDRESS_UNIT_BYTES) &
        PSX_SPU_REVERB_BUFFER_END_ADDRESS;
    return address < base ? base : address;
}

void psx_spu_reverb_advance(PsxSpu *spu)
{
    uint32_t base;
    if (!spu || !spu->initialized)
        return;
    base = ((uint32_t)load_le16(&spu->registers[
        PSX_SPU_REVERB_WORK_AREA_START_OFFSET]) *
        PSX_SPU_REVERB_ADDRESS_UNIT_BYTES) &
        PSX_SPU_REVERB_BUFFER_END_ADDRESS;
    spu->reverb_buffer_address =
        (spu->reverb_buffer_address + 2u) &
        PSX_SPU_REVERB_BUFFER_END_ADDRESS;
    if (spu->reverb_buffer_address < base)
        spu->reverb_buffer_address = base;
}



static int32_t reverb_multiply_q15_wide(int32_t sample, int16_t volume)
{
    return (int32_t)(((int64_t)sample * (int64_t)volume) / 0x8000);
}

static uint32_t reverb_previous_sample_address(const PsxSpu *spu,
                                                uint16_t relative_address)
{
    const uint32_t base =
        ((uint32_t)load_le16(&spu->registers[
            PSX_SPU_REVERB_WORK_AREA_START_OFFSET]) *
         PSX_SPU_REVERB_ADDRESS_UNIT_BYTES) &
        PSX_SPU_REVERB_BUFFER_END_ADDRESS;
    const uint32_t address =
        psx_spu_reverb_resolve_address(spu, relative_address);
    return address >= base + 2u ? address - 2u : base;
}

static int16_t reverb_read_absolute(const PsxSpu *spu, uint32_t address)
{
    return (int16_t)load_le16(&spu->ram[address]);
}

static void reverb_write_absolute(PsxSpu *spu, uint32_t address,
                                  int32_t value)
{
    store_le16(&spu->ram[address],
               (uint16_t)psx_spu_reverb_saturate(value));
}

static void reverb_process_reflection(PsxSpu *spu, int32_t input,
                                      uint16_t destination_offset,
                                      uint16_t delay_offset,
                                      int16_t wall_volume,
                                      int16_t iir_volume)
{
    const uint16_t destination_relative = load_le16(
        &spu->registers[destination_offset]);
    const uint16_t delay_relative = load_le16(
        &spu->registers[delay_offset]);
    const uint32_t destination = psx_spu_reverb_resolve_address(
        spu, destination_relative);
    const uint32_t previous_address = reverb_previous_sample_address(
        spu, destination_relative);
    const int32_t delayed = reverb_read_absolute(
        spu, psx_spu_reverb_resolve_address(spu, delay_relative));
    const int32_t previous = reverb_read_absolute(spu, previous_address);
    const int32_t wall = reverb_multiply_q15_wide(delayed, wall_volume);
    const int32_t reflected = reverb_multiply_q15_wide(
        input + wall - previous, iir_volume) + previous;

    reverb_write_absolute(spu, destination, reflected);
}

bool psx_spu_reverb_process_reflections(PsxSpu *spu, int16_t left_input,
                                       int16_t right_input)
{
    int16_t input_left_volume;
    int16_t input_right_volume;
    int16_t wall_volume;
    int16_t iir_volume;
    int32_t input_left;
    int32_t input_right;

    if (!spu || !spu->initialized)
        return false;

    input_left_volume = (int16_t)load_le16(&spu->registers[
        PSX_SPU_REVERB_CONFIG_VLIN_OFFSET]);
    input_right_volume = (int16_t)load_le16(&spu->registers[
        PSX_SPU_REVERB_CONFIG_VRIN_OFFSET]);
    wall_volume = (int16_t)load_le16(&spu->registers[
        PSX_SPU_REVERB_CONFIG_VWALL_OFFSET]);
    iir_volume = (int16_t)load_le16(&spu->registers[
        PSX_SPU_REVERB_CONFIG_VIIR_OFFSET]);

    input_left = psx_spu_reverb_multiply_q15(left_input,
                                              input_left_volume);
    input_right = psx_spu_reverb_multiply_q15(right_input,
                                               input_right_volume);

    /* The DIFF stage crosses channels: left reads dRDIFF and right dLDIFF. */
    reverb_process_reflection(spu, input_left,
        PSX_SPU_REVERB_CONFIG_MLSAME_OFFSET,
        PSX_SPU_REVERB_CONFIG_DLSAME_OFFSET, wall_volume, iir_volume);
    reverb_process_reflection(spu, input_right,
        PSX_SPU_REVERB_CONFIG_MRSAME_OFFSET,
        PSX_SPU_REVERB_CONFIG_DRSAME_OFFSET, wall_volume, iir_volume);
    reverb_process_reflection(spu, input_left,
        PSX_SPU_REVERB_CONFIG_MLDIFF_OFFSET,
        PSX_SPU_REVERB_CONFIG_DRDIFF_OFFSET, wall_volume, iir_volume);
    reverb_process_reflection(spu, input_right,
        PSX_SPU_REVERB_CONFIG_MRDIFF_OFFSET,
        PSX_SPU_REVERB_CONFIG_DLDIFF_OFFSET, wall_volume, iir_volume);
    return true;
}

bool psx_spu_reverb_process_comb(const PsxSpu *spu,
                                   int16_t *left_output,
                                   int16_t *right_output)
{
    static const uint16_t left_taps[4] = {
        PSX_SPU_REVERB_CONFIG_MLCOMB1_OFFSET,
        PSX_SPU_REVERB_CONFIG_MLCOMB2_OFFSET,
        PSX_SPU_REVERB_CONFIG_MLCOMB3_OFFSET,
        PSX_SPU_REVERB_CONFIG_MLCOMB4_OFFSET
    };
    static const uint16_t right_taps[4] = {
        PSX_SPU_REVERB_CONFIG_MRCOMB1_OFFSET,
        PSX_SPU_REVERB_CONFIG_MRCOMB2_OFFSET,
        PSX_SPU_REVERB_CONFIG_MRCOMB3_OFFSET,
        PSX_SPU_REVERB_CONFIG_MRCOMB4_OFFSET
    };
    static const uint16_t volumes[4] = {
        PSX_SPU_REVERB_CONFIG_VCOMB1_OFFSET,
        PSX_SPU_REVERB_CONFIG_VCOMB2_OFFSET,
        PSX_SPU_REVERB_CONFIG_VCOMB3_OFFSET,
        PSX_SPU_REVERB_CONFIG_VCOMB4_OFFSET
    };
    int64_t left_sum = 0;
    int64_t right_sum = 0;

    if (!spu || !spu->initialized || !left_output || !right_output)
        return false;

    /* Each channel has four independently delayed taps. Accumulate the
     * signed Q15 products at full width and saturate only at the output. */
    for (unsigned i = 0u; i < 4u; ++i) {
        const uint16_t left_address = load_le16(&spu->registers[left_taps[i]]);
        const uint16_t right_address = load_le16(&spu->registers[right_taps[i]]);
        const int16_t coefficient = (int16_t)load_le16(
            &spu->registers[volumes[i]]);
        const int32_t left_sample = reverb_read_absolute(
            spu, psx_spu_reverb_resolve_address(spu, left_address));
        const int32_t right_sample = reverb_read_absolute(
            spu, psx_spu_reverb_resolve_address(spu, right_address));
        left_sum += reverb_multiply_q15_wide(left_sample, coefficient);
        right_sum += reverb_multiply_q15_wide(right_sample, coefficient);
    }

    *left_output = psx_spu_reverb_saturate(
        left_sum > INT32_MAX ? INT32_MAX :
        left_sum < INT32_MIN ? INT32_MIN : (int32_t)left_sum);
    *right_output = psx_spu_reverb_saturate(
        right_sum > INT32_MAX ? INT32_MAX :
        right_sum < INT32_MIN ? INT32_MIN : (int32_t)right_sum);
    return true;
}

bool psx_spu_reverb_read16(const PsxSpu *spu, uint16_t relative_address,
                           int16_t *value)
{
    uint32_t address;
    if (!spu || !spu->initialized || !value)
        return false;
    address = psx_spu_reverb_resolve_address(spu, relative_address);
    *value = (int16_t)load_le16(&spu->ram[address]);
    return true;
}

bool psx_spu_reverb_write16(PsxSpu *spu, uint16_t relative_address,
                            int16_t value)
{
    uint32_t address;
    if (!spu || !spu->initialized)
        return false;
    address = psx_spu_reverb_resolve_address(spu, relative_address);
    store_le16(&spu->ram[address], (uint16_t)value);
    return true;
}

bool psx_spu_mix_block(PsxSpu *spu, int16_t *interleaved_stereo,
                       size_t frame_capacity, size_t *frames_written)
{
    if (frames_written)
        *frames_written = 0u;
    if (!spu || !spu->initialized ||
        (!interleaved_stereo && frame_capacity != 0u) || !frames_written)
        return false;

    for (size_t frame = 0u; frame < frame_capacity; ++frame) {
        int16_t left = 0;
        int16_t right = 0;
        (void)psx_spu_mix_sample(spu, &left, &right);
        interleaved_stereo[frame * 2u] = left;
        interleaved_stereo[frame * 2u + 1u] = right;
    }
    *frames_written = frame_capacity;
    return true;
}

uint32_t psx_spu_active_voice_mask(const PsxSpu *spu)
{
    return (spu && spu->initialized) ? spu->active_voices : 0u;
}

bool psx_spu_read8(PsxSpu *spu, uint32_t address, uint8_t *value)
{
    uint16_t word;
    uint32_t aligned = address & ~1u;
    if (!value || aligned == PSX_SPU_BASE_ADDR +
        PSX_SPU_TRANSFER_DATA_OFFSET ||
        !psx_spu_read16(spu, aligned, &word)) return false;
    *value = (uint8_t)(word >> ((address & 1u) * 8u));
    return true;
}

bool psx_spu_write8(PsxSpu *spu, uint32_t address, uint8_t value)
{
    uint16_t word;
    const uint32_t aligned = address & ~1u;
    const unsigned shift = (address & 1u) * 8u;
    if (aligned == PSX_SPU_BASE_ADDR + PSX_SPU_TRANSFER_DATA_OFFSET ||
        !psx_spu_read16(spu, aligned, &word)) return false;
    word = (uint16_t)((word & (uint16_t)~(0xffu << shift)) |
                      ((uint16_t)value << shift));
    return psx_spu_write16(spu, aligned, word);
}
