#ifndef DREAMPSX_MEMORY_H
#define DREAMPSX_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSX_RAM_SIZE       (2u * 1024u * 1024u)
#define PSX_RAM_MIRROR_END 0x00800000u
#define PSX_SCRATCH_BASE   0x1f800000u
#define PSX_SCRATCH_SIZE   0x00000400u
#define PSX_BIOS_BASE      0x1fc00000u
#define PSX_BIOS_SIZE      0x00080000u

typedef struct {
    uint8_t *ram;              /* Caller-owned, exactly 2 MiB. */
    const uint8_t *bios;       /* Caller-owned, exactly 512 KiB. */
    uint8_t scratchpad[PSX_SCRATCH_SIZE];
    bool initialized;
} PsxMemory;

/* RAM and BIOS storage remain owned by the caller and must outlive PsxMemory.
 * BIOS is read-only. The supplied sizes must match the hardware regions.
 */
bool psx_memory_init(PsxMemory *memory,
                     uint8_t *ram, size_t ram_size,
                     const uint8_t *bios, size_t bios_size);

bool psx_memory_read8(const PsxMemory *memory, uint32_t address, uint8_t *value);
bool psx_memory_read16(const PsxMemory *memory, uint32_t address, uint16_t *value);
bool psx_memory_read32(const PsxMemory *memory, uint32_t address, uint32_t *value);
bool psx_memory_write8(PsxMemory *memory, uint32_t address, uint8_t value);
bool psx_memory_write16(PsxMemory *memory, uint32_t address, uint16_t value);
bool psx_memory_write32(PsxMemory *memory, uint32_t address, uint32_t value);

#endif
