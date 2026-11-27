#include "dreampsx/psx_exe.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    uint8_t image[PSX_EXE_HEADER_SIZE + 16u];
    PsxMemory memory;
    PsxExeInfo info;
    uint32_t value;

    assert(ram && bios);
    memset(&memory, 0, sizeof(memory));
    assert(psx_memory_init(&memory, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE));
    memset(image, 0, sizeof(image));
    memcpy(image, "PS-X EXE", 8u);
    put32(image + 0x10u, 0x80010000u);
    put32(image + 0x14u, 0x80020000u);
    put32(image + 0x18u, 0x80010000u);
    put32(image + 0x1cu, 16u);
    put32(image + 0x28u, 0x80011000u);
    put32(image + 0x2cu, 8u);
    put32(image + 0x30u, 0x801fff00u);
    put32(image + 0x34u, 0x100u);
    for (unsigned i = 0u; i < 16u; ++i)
        image[PSX_EXE_HEADER_SIZE + i] = (uint8_t)(0xa0u + i);
    memset(ram + 0x11000u, 0x5au, 8u);

    assert(psx_exe_load(&memory, image, sizeof(image), &info));
    assert(info.entry_pc == 0x80010000u);
    assert(info.global_pointer == 0x80020000u);
    assert(info.load_address == 0x80010000u && info.file_size == 16u);
    assert(info.initial_stack_pointer == 0x80200000u);
    assert(psx_memory_read32(&memory, 0x80010000u, &value));
    assert(value == 0xa3a2a1a0u);
    for (unsigned i = 0u; i < 8u; ++i)
        assert(ram[0x11000u + i] == 0u);

    /* A malformed/out-of-range image is rejected without touching RAM. */
    memset(ram + 0x10000u, 0x3cu, 16u);
    put32(image + 0x18u, 0x801ffff8u);
    put32(image + 0x10u, 0x801ffff8u);
    put32(image + 0x1cu, 16u);
    assert(!psx_exe_load(&memory, image, sizeof(image), &info));
    for (unsigned i = 0u; i < 16u; ++i)
        assert(ram[0x10000u + i] == 0x3cu);

    put32(image + 0x18u, 0x80010000u);
    put32(image + 0x10u, 0x80010002u); /* unaligned entry point */
    assert(!psx_exe_load(&memory, image, sizeof(image), &info));
    put32(image + 0x10u, 0x80010000u);
    put32(image + 0x2cu, 0x100u);
    put32(image + 0x28u, 0x801fff80u); /* BSS crosses RAM end */
    assert(!psx_exe_load(&memory, image, sizeof(image), &info));
    assert(!psx_exe_load(&memory, image, PSX_EXE_HEADER_SIZE - 1u, &info));
    image[0] = 'X';
    assert(!psx_exe_load(&memory, image, sizeof(image), &info));

    free(bios);
    free(ram);
    puts("PS-X EXE loader tests: OK");
    return 0;
}
