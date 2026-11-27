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

    if (offset == PSX_SPU_KEY_ON_LOW_OFFSET) {
        spu->active_voices |= value;
        return true;
    }
    if (offset == PSX_SPU_KEY_ON_HIGH_OFFSET) {
        spu->active_voices |= (uint32_t)(value & 0x00ffu) << 16u;
        return true;
    }
    if (offset == PSX_SPU_KEY_OFF_LOW_OFFSET) {
        spu->active_voices &= ~(uint32_t)value;
        return true;
    }
    if (offset == PSX_SPU_KEY_OFF_HIGH_OFFSET) {
        spu->active_voices &= ~((uint32_t)(value & 0x00ffu) << 16u);
        return true;
    }
    if (offset == PSX_SPU_CONTROL_OFFSET) {
        spu->control = value;
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
