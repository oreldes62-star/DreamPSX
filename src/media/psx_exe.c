#include "dreampsx/psx_exe.h"

#include <string.h>

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static bool physical_ram_range(uint32_t address, uint32_t size,
                               uint32_t *physical)
{
    uint32_t base = address;
    if ((address >= 0x80000000u && address <= 0x9fffffffu) ||
        (address >= 0xa0000000u && address <= 0xbfffffffu))
        base = address & 0x1fffffffu;
    if (base >= PSX_RAM_SIZE || size > PSX_RAM_SIZE - base)
        return false;
    *physical = base;
    return true;
}

bool psx_exe_load(PsxMemory *memory, const uint8_t *image, size_t image_size,
                  PsxExeInfo *info)
{
    PsxExeInfo parsed;
    uint32_t load_physical, bss_physical = 0u;
    uint32_t stack_physical;
    if (!memory || !memory->initialized || !image || !info ||
        image_size < PSX_EXE_HEADER_SIZE ||
        memcmp(image, "PS-X EXE", 8u) != 0)
        return false;

    parsed.entry_pc = read_le32(image + 0x10u);
    parsed.global_pointer = read_le32(image + 0x14u);
    parsed.load_address = read_le32(image + 0x18u);
    parsed.file_size = read_le32(image + 0x1cu);
    parsed.bss_address = read_le32(image + 0x28u);
    parsed.bss_size = read_le32(image + 0x2cu);
    parsed.stack_base = read_le32(image + 0x30u);
    parsed.stack_offset = read_le32(image + 0x34u);
    if (parsed.stack_offset > UINT32_MAX - parsed.stack_base)
        return false;
    parsed.initial_stack_pointer = parsed.stack_base + parsed.stack_offset;

    if (parsed.file_size == 0u ||
        (uint64_t)PSX_EXE_HEADER_SIZE + parsed.file_size > image_size ||
        (parsed.entry_pc & 3u) != 0u ||
        !physical_ram_range(parsed.load_address, parsed.file_size,
                            &load_physical) ||
        parsed.entry_pc < parsed.load_address ||
        (uint64_t)parsed.entry_pc >=
            (uint64_t)parsed.load_address + parsed.file_size)
        return false;

    if (parsed.bss_size != 0u &&
        !physical_ram_range(parsed.bss_address, parsed.bss_size,
                            &bss_physical))
        return false;
    if (parsed.initial_stack_pointer != 0u &&
        !physical_ram_range(parsed.initial_stack_pointer - 1u, 1u,
                            &stack_physical))
        return false;

    /* All ranges have been checked, so this copy cannot partially fail for
     * a valid initialized memory object. Use physical RAM to avoid aliases. */
    memcpy(memory->ram + load_physical, image + PSX_EXE_HEADER_SIZE,
           parsed.file_size);
    if (parsed.bss_size != 0u)
        memset(memory->ram + bss_physical, 0, parsed.bss_size);

    *info = parsed;
    return true;
}
