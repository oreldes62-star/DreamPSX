#include "dreampsx/spu.h"

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
        *value = spu->voices[offset / PSX_SPU_VOICE_REGISTER_BYTES].current_volume;
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
            voice->block_valid = false;
        }
        spu->active_voices |= mask;
        return true;
    }
    if (offset == PSX_SPU_KEY_OFF_LOW_OFFSET ||
        offset == PSX_SPU_KEY_OFF_HIGH_OFFSET) {
        const uint32_t mask = offset == PSX_SPU_KEY_OFF_LOW_OFFSET ?
            (uint32_t)value : ((uint32_t)(value & 0x00ffu) << 16u);
        spu->active_voices &= ~mask;
        for (unsigned i = 0u; i < PSX_SPU_VOICE_COUNT; ++i)
            if ((mask & (1u << i)) != 0u)
                spu->voices[i].block_valid = false;
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
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; ++i)
            block[i] = spu->ram[(voice->playback_address + i) &
                                (PSX_SPU_RAM_BYTES - 1u)];
        if (!psx_spu_adpcm_decode_block(&voice->adpcm_state, block,
                                        voice->decoded_samples,
                                        &voice->block_flags)) {
            spu->active_voices &= ~(1u << voice_index);
            voice->block_valid = false;
            return false;
        }
        voice->playback_address = (voice->playback_address +
            PSX_SPU_ADPCM_BLOCK_BYTES) & (PSX_SPU_RAM_BYTES - 1u);
        voice->sample_index = 0u;
        voice->block_valid = true;
    }

    *sample = voice->decoded_samples[voice->sample_index++];
    if (voice->sample_index == PSX_SPU_ADPCM_BLOCK_SAMPLES &&
        (voice->block_flags & 0x01u) != 0u) {
        spu->active_voices &= ~(1u << voice_index);
        voice->block_valid = false;
    }
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
