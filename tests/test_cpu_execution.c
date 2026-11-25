#include "dreampsx/cpu.h"
#include "dreampsx/machine.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void put_word(uint8_t *ram, unsigned address, uint32_t value)
{
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
    ram[address + 2u] = (uint8_t)(value >> 16);
    ram[address + 3u] = (uint8_t)(value >> 24);
}

int main(void)
{
    PsxMachine machine;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    assert(ram && bios);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE));

    put_word(ram, 0x100u, 0x24080005u); /* ADDIU t0,zero,5 */
    put_word(ram, 0x104u, 0x24090007u); /* ADDIU t1,zero,7 */
    put_word(ram, 0x108u, 0x01095021u); /* ADDU t2,t0,t1 */
    put_word(ram, 0x10cu, 0x114a0001u); /* BEQ t2,t2,+1 */
    put_word(ram, 0x110u, 0x240b0009u); /* delay slot: t3=9 */
    put_word(ram, 0x114u, 0x240b0063u); /* skipped: t3=99 */
    put_word(ram, 0x118u, 0x240c000bu); /* target: t4=11 */

    machine.cpu.pc = 0x100u;
    machine.cpu.next_pc = 0x104u;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 5u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 7u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 12u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.pc == 0x110u && machine.cpu.next_pc == 0x114u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 9u);
    assert(machine.cpu.pc == 0x114u && machine.cpu.next_pc == 0x118u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 12u) == 11u);
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 9u);

    psx_cpu_write_gpr(&machine.cpu, 8u, 0x180u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0xfeed1234u);
    put_word(ram, 0x11cu, 0xad090000u); /* SW t1,0(t0) */
    put_word(ram, 0x120u, 0x8d0a0000u); /* LW t2,0(t0) */
    machine.cpu.pc = 0x11cu;
    machine.cpu.next_pc = 0x120u;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0xfeed1234u);

    free(bios); free(ram);
    puts("DreamPSX R3000A execution tests: OK");
    return 0;
}
