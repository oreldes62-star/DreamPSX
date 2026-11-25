#include "dreampsx/memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    PsxMemory memory;
    uint8_t b;
    uint16_t h;
    uint32_t w;
    assert(ram && bios);
    memset(&memory, 0, sizeof(memory));
    bios[0] = 0x5au;
    bios[1] = 0xa5u;

    assert(psx_memory_init(&memory, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE));
    assert(!psx_memory_init(&memory, ram, PSX_RAM_SIZE - 1u,
                            bios, PSX_BIOS_SIZE));

    /* Little-endian word and all RAM mirror/segment aliases. */
    assert(psx_memory_write32(&memory, 0x00000000u, 0x12345678u));
    assert(psx_memory_read32(&memory, 0x00200000u, &w) && w == 0x12345678u);
    assert(psx_memory_read32(&memory, 0x80000000u, &w) && w == 0x12345678u);
    assert(psx_memory_read32(&memory, 0xa0000000u, &w) && w == 0x12345678u);

    /* Scratchpad is writable and independent from main RAM. */
    assert(psx_memory_write16(&memory, PSX_SCRATCH_BASE, 0xbeefu));
    assert(psx_memory_read16(&memory, PSX_SCRATCH_BASE, &h) && h == 0xbeefu);
    assert(psx_memory_read8(&memory, 0u, &b) && b == 0x78u);

    /* BIOS is readable, and writes are intentionally ignored. */
    assert(psx_memory_read16(&memory, PSX_BIOS_BASE, &h) && h == 0xa55au);
    assert(psx_memory_write8(&memory, PSX_BIOS_BASE, 0u));
    assert(psx_memory_read8(&memory, PSX_BIOS_BASE, &b) && b == 0x5au);

    /* Reject misalignment and unmapped locations without partial writes. */
    assert(!psx_memory_read16(&memory, 1u, &h));
    assert(!psx_memory_write32(&memory, 2u, 0u));
    assert(!psx_memory_read8(&memory, 0x1f801000u, &b));
    assert(psx_memory_write8(&memory, PSX_SCRATCH_BASE +
                             PSX_SCRATCH_SIZE - 1u, 0x33u));
    assert(!psx_memory_write16(&memory, PSX_SCRATCH_BASE +
                               PSX_SCRATCH_SIZE - 2u, 0x7788u));
    assert(psx_memory_read8(&memory, PSX_SCRATCH_BASE +
                            PSX_SCRATCH_SIZE - 1u, &b) && b == 0x33u);

    free(bios);
    free(ram);
    puts("DreamPSX memory tests: OK");
    return 0;
}
