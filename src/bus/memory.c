#include "dreampsx/memory.h"

static uint32_t physical_address(uint32_t address)
{
    /* KSEG0 and KSEG1 are direct-mapped aliases of physical memory. */
    if ((address >= 0x80000000u && address <= 0x9fffffffu) ||
        (address >= 0xa0000000u && address <= 0xbfffffffu)) {
        return address & 0x1fffffffu;
    }
    return address;
}

static bool locate(const PsxMemory *memory, uint32_t address,
                   const uint8_t **read_ptr, uint8_t **write_ptr)
{
    if (!memory || !memory->initialized) return false;
    address = physical_address(address);

    if (address < PSX_RAM_MIRROR_END) {
        const uint32_t offset = address & (PSX_RAM_SIZE - 1u);
        if (read_ptr) *read_ptr = memory->ram + offset;
        if (write_ptr) *write_ptr = memory->ram + offset;
        return true;
    }
    if (address >= PSX_SCRATCH_BASE &&
        address < PSX_SCRATCH_BASE + PSX_SCRATCH_SIZE) {
        const uint32_t offset = address - PSX_SCRATCH_BASE;
        if (read_ptr) *read_ptr = memory->scratchpad + offset;
        if (write_ptr) *write_ptr = memory->scratchpad + offset;
        return true;
    }
    if (address >= PSX_BIOS_BASE &&
        address < PSX_BIOS_BASE + PSX_BIOS_SIZE) {
        const uint32_t offset = address - PSX_BIOS_BASE;
        if (read_ptr) *read_ptr = memory->bios + offset;
        if (write_ptr) *write_ptr = NULL;
        return true;
    }
    return false;
}

bool psx_memory_init(PsxMemory *memory,
                     uint8_t *ram, size_t ram_size,
                     const uint8_t *bios, size_t bios_size)
{
    if (!memory || !ram || !bios ||
        ram_size != PSX_RAM_SIZE || bios_size != PSX_BIOS_SIZE) {
        return false;
    }
    memory->ram = ram;
    memory->bios = bios;
    for (size_t i = 0; i < PSX_SCRATCH_SIZE; ++i)
        memory->scratchpad[i] = 0u;
    memory->initialized = true;
    return true;
}

static bool span_is_mapped(const PsxMemory *memory, uint32_t address,
                             unsigned length)
{
    for (unsigned i = 0; i < length; ++i) {
        if (!locate(memory, address + i, NULL, NULL)) return false;
    }
    return true;
}

bool psx_memory_read8(const PsxMemory *memory, uint32_t address, uint8_t *value)
{
    const uint8_t *ptr = NULL;
    if (!value || !locate(memory, address, &ptr, NULL)) return false;
    *value = *ptr;
    return true;
}

bool psx_memory_read16(const PsxMemory *memory, uint32_t address, uint16_t *value)
{
    uint8_t lo, hi;
    if (!value || (address & 1u) ||
        !psx_memory_read8(memory, address, &lo) ||
        !psx_memory_read8(memory, address + 1u, &hi)) return false;
    *value = (uint16_t)((uint16_t)lo | ((uint16_t)hi << 8));
    return true;
}

bool psx_memory_read32(const PsxMemory *memory, uint32_t address, uint32_t *value)
{
    uint16_t lo, hi;
    if (!value || (address & 3u) ||
        !psx_memory_read16(memory, address, &lo) ||
        !psx_memory_read16(memory, address + 2u, &hi)) return false;
    *value = (uint32_t)lo | ((uint32_t)hi << 16);
    return true;
}

bool psx_memory_write8(PsxMemory *memory, uint32_t address, uint8_t value)
{
    uint8_t *ptr = NULL;
    if (!locate(memory, address, NULL, &ptr)) return false;
    if (ptr) *ptr = value; /* Writes to the ROM window are ignored. */
    return true;
}

bool psx_memory_write16(PsxMemory *memory, uint32_t address, uint16_t value)
{
    if ((address & 1u) || !span_is_mapped(memory, address, 2u))
        return false;
    if (!psx_memory_write8(memory, address, (uint8_t)value) ||
        !psx_memory_write8(memory, address + 1u, (uint8_t)(value >> 8)))
        return false;
    return true;
}

bool psx_memory_write32(PsxMemory *memory, uint32_t address, uint32_t value)
{
    if ((address & 3u) || !span_is_mapped(memory, address, 4u))
        return false;
    if (!psx_memory_write16(memory, address, (uint16_t)value) ||
        !psx_memory_write16(memory, address + 2u, (uint16_t)(value >> 16)))
        return false;
    return true;
}
