#include "dreampsx/cpu.h"
#include "dreampsx/machine.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

static void put_word(uint8_t *ram, unsigned address, uint32_t value)
{
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
    ram[address + 2u] = (uint8_t)(value >> 16);
    ram[address + 3u] = (uint8_t)(value >> 24);
}

static void execute_at(PsxMachine *m, uint8_t *ram, unsigned pc,
                       uint32_t instruction)
{
    put_word(ram, pc, instruction);
    m->cpu.pc = pc;
    m->cpu.next_pc = pc + 4u;
    assert(psx_cpu_step(&m->cpu, &m->bus));
}

int main(void)
{
    PsxMachine m;
    static uint16_t gpu_vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    assert(ram && bios);
    assert(psx_machine_init(&m, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE,
                            gpu_vram, PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* Signed and unsigned multiply, plus HI/LO transfers. */
    psx_cpu_write_gpr(&m.cpu, 8u, 7u);
    psx_cpu_write_gpr(&m.cpu, 9u, 0xfffffffeu);
    execute_at(&m, ram, 0x100u, 0x01090018u); /* MULT t0,t1 */
    execute_at(&m, ram, 0x104u, 0x00005012u); /* MFLO t2 */
    execute_at(&m, ram, 0x108u, 0x00005810u); /* MFHI t3 */
    assert(psx_cpu_read_gpr(&m.cpu, 10u) == 0xfffffff2u);
    assert(psx_cpu_read_gpr(&m.cpu, 11u) == 0xffffffffu);

    psx_cpu_write_gpr(&m.cpu, 8u, 20u);
    psx_cpu_write_gpr(&m.cpu, 9u, 3u);
    execute_at(&m, ram, 0x10cu, 0x0109001au); /* DIV t0,t1 */
    execute_at(&m, ram, 0x110u, 0x00006012u); /* MFLO t4 */
    execute_at(&m, ram, 0x114u, 0x00006810u); /* MFHI t5 */
    assert(psx_cpu_read_gpr(&m.cpu, 12u) == 6u);
    assert(psx_cpu_read_gpr(&m.cpu, 13u) == 2u);

    /* Byte/halfword stores and signed/unsigned loads. */
    psx_cpu_write_gpr(&m.cpu, 8u, 0x200u);
    psx_cpu_write_gpr(&m.cpu, 9u, 0x80u);
    execute_at(&m, ram, 0x118u, 0xa1090000u); /* SB t1,0(t0) */
    execute_at(&m, ram, 0x11cu, 0x810a0000u); /* LB t2,0(t0) */
    execute_at(&m, ram, 0x120u, 0x910b0000u); /* LBU t3,0(t0) */
    assert(psx_cpu_read_gpr(&m.cpu, 10u) == 0xffffff80u);
    execute_at(&m, ram, 0x13cu, 0x00000000u); /* NOP: retire LBU */
    assert(psx_cpu_read_gpr(&m.cpu, 11u) == 0x80u);

    psx_cpu_write_gpr(&m.cpu, 9u, 0x8001u);
    execute_at(&m, ram, 0x124u, 0xa5090002u); /* SH t1,2(t0) */
    execute_at(&m, ram, 0x128u, 0x850c0002u); /* LH t4,2(t0) */
    execute_at(&m, ram, 0x12cu, 0x950d0002u); /* LHU t5,2(t0) */
    assert(psx_cpu_read_gpr(&m.cpu, 12u) == 0xffff8001u);
    execute_at(&m, ram, 0x140u, 0x00000000u); /* NOP: retire LHU */
    assert(psx_cpu_read_gpr(&m.cpu, 13u) == 0x8001u);

    /* Division by zero follows the R3000A quotient/remainder convention. */
    psx_cpu_write_gpr(&m.cpu, 8u, 5u);
    psx_cpu_write_gpr(&m.cpu, 9u, 0u);
    execute_at(&m, ram, 0x130u, 0x0109001au);
    execute_at(&m, ram, 0x134u, 0x00007012u); /* MFLO t6 */
    execute_at(&m, ram, 0x138u, 0x00007810u); /* MFHI t7 */
    assert(psx_cpu_read_gpr(&m.cpu, 14u) == 0xffffffffu);
    assert(psx_cpu_read_gpr(&m.cpu, 15u) == 5u);

    /* Signed division truncates toward zero; remainder keeps dividend sign. */
    psx_cpu_write_gpr(&m.cpu, 8u, 0xfffffff9u); /* -7 */
    psx_cpu_write_gpr(&m.cpu, 9u, 3u);
    execute_at(&m, ram, 0x144u, 0x0109001au); /* DIV t0,t1 */
    execute_at(&m, ram, 0x148u, 0x00008012u); /* MFLO t8 */
    execute_at(&m, ram, 0x14cu, 0x00008810u); /* MFHI t9 */
    assert(psx_cpu_read_gpr(&m.cpu, 16u) == 0xfffffffeu); /* -2 */
    assert(psx_cpu_read_gpr(&m.cpu, 17u) == 0xffffffffu); /* -1 */

    /* R3000A signed overflow corner: INT32_MIN / -1 is defined here. */
    psx_cpu_write_gpr(&m.cpu, 8u, 0x80000000u);
    psx_cpu_write_gpr(&m.cpu, 9u, 0xffffffffu);
    execute_at(&m, ram, 0x150u, 0x0109001au);
    execute_at(&m, ram, 0x154u, 0x00008012u);
    execute_at(&m, ram, 0x158u, 0x00008810u);
    assert(psx_cpu_read_gpr(&m.cpu, 16u) == 0x80000000u);
    assert(psx_cpu_read_gpr(&m.cpu, 17u) == 0u);

    /* Unsigned division and multiplication preserve all 64 result bits. */
    psx_cpu_write_gpr(&m.cpu, 8u, 0xffffffffu);
    psx_cpu_write_gpr(&m.cpu, 9u, 3u);
    execute_at(&m, ram, 0x15cu, 0x0109001bu); /* DIVU t0,t1 */
    execute_at(&m, ram, 0x160u, 0x00008012u);
    execute_at(&m, ram, 0x164u, 0x00008810u);
    assert(psx_cpu_read_gpr(&m.cpu, 16u) == 0x55555555u);
    assert(psx_cpu_read_gpr(&m.cpu, 17u) == 0u);

    psx_cpu_write_gpr(&m.cpu, 8u, 0xffffffffu);
    psx_cpu_write_gpr(&m.cpu, 9u, 0xffffffffu);
    execute_at(&m, ram, 0x168u, 0x01090019u); /* MULTU t0,t1 */
    execute_at(&m, ram, 0x16cu, 0x00008012u);
    execute_at(&m, ram, 0x170u, 0x00008810u);
    assert(psx_cpu_read_gpr(&m.cpu, 16u) == 1u);
    assert(psx_cpu_read_gpr(&m.cpu, 17u) == 0xfffffffeu);

    /* Unsigned division by zero returns all-ones quotient and dividend remainder. */
    psx_cpu_write_gpr(&m.cpu, 8u, 0x87654321u);
    psx_cpu_write_gpr(&m.cpu, 9u, 0u);
    execute_at(&m, ram, 0x174u, 0x0109001bu); /* DIVU t0,t1 */
    execute_at(&m, ram, 0x178u, 0x00008012u);
    execute_at(&m, ram, 0x17cu, 0x00008810u);
    assert(psx_cpu_read_gpr(&m.cpu, 16u) == 0xffffffffu);
    assert(psx_cpu_read_gpr(&m.cpu, 17u) == 0x87654321u);

    /* Signed multiply writes the two's-complement 64-bit product to HI/LO. */
    psx_cpu_write_gpr(&m.cpu, 8u, 0xfffffffeu); /* -2 */
    psx_cpu_write_gpr(&m.cpu, 9u, 3u);
    execute_at(&m, ram, 0x180u, 0x01090018u); /* MULT t0,t1 */
    execute_at(&m, ram, 0x184u, 0x00008012u); /* MFLO t8 */
    execute_at(&m, ram, 0x188u, 0x00008810u); /* MFHI t9 */
    assert(psx_cpu_read_gpr(&m.cpu, 16u) == 0xfffffffau);
    assert(psx_cpu_read_gpr(&m.cpu, 17u) == 0xffffffffu);

    /* R3000A signed DIV by zero: negative dividend produces quotient 1. */
    psx_cpu_write_gpr(&m.cpu, 8u, 0x87654321u);
    psx_cpu_write_gpr(&m.cpu, 9u, 0u);
    execute_at(&m, ram, 0x18cu, 0x0109001au); /* DIV t0,t1 */
    execute_at(&m, ram, 0x190u, 0x00008012u);
    execute_at(&m, ram, 0x194u, 0x00008810u);
    assert(psx_cpu_read_gpr(&m.cpu, 16u) == 1u);
    assert(psx_cpu_read_gpr(&m.cpu, 17u) == 0x87654321u);

    /* Positive dividend divided by zero produces quotient -1. */
    psx_cpu_write_gpr(&m.cpu, 8u, 0x12345678u);
    psx_cpu_write_gpr(&m.cpu, 9u, 0u);
    execute_at(&m, ram, 0x198u, 0x0109001au);
    execute_at(&m, ram, 0x19cu, 0x00008012u);
    execute_at(&m, ram, 0x1a0u, 0x00008810u);
    assert(psx_cpu_read_gpr(&m.cpu, 16u) == 0xffffffffu);
    assert(psx_cpu_read_gpr(&m.cpu, 17u) == 0x12345678u);

    free(bios);
    free(ram);
    return 0;
}
