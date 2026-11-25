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
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    assert(ram && bios);
    assert(psx_machine_init(&m, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE));

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
    assert(psx_cpu_read_gpr(&m.cpu, 11u) == 0x80u);

    psx_cpu_write_gpr(&m.cpu, 9u, 0x8001u);
    execute_at(&m, ram, 0x124u, 0xa1090002u); /* SH t1,2(t0) */
    execute_at(&m, ram, 0x128u, 0x850c0002u); /* LH t4,2(t0) */
    execute_at(&m, ram, 0x12cu, 0x950d0002u); /* LHU t5,2(t0) */
    assert(psx_cpu_read_gpr(&m.cpu, 12u) == 0xffff8001u);
    assert(psx_cpu_read_gpr(&m.cpu, 13u) == 0x8001u);

    /* Division by zero follows the R3000A quotient/remainder convention. */
    psx_cpu_write_gpr(&m.cpu, 8u, 5u);
    psx_cpu_write_gpr(&m.cpu, 9u, 0u);
    execute_at(&m, ram, 0x130u, 0x0109001au);
    execute_at(&m, ram, 0x134u, 0x00007012u); /* MFLO t6 */
    execute_at(&m, ram, 0x138u, 0x00007810u); /* MFHI t7 */
    assert(psx_cpu_read_gpr(&m.cpu, 14u) == 0xffffffffu);
    assert(psx_cpu_read_gpr(&m.cpu, 15u) == 5u);

    free(bios);
    free(ram);
    return 0;
}
