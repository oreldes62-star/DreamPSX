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


    /* A not-taken branch still has a delay slot for exception EPC/BD tracking. */
    put_word(ram, 0x140u, 0x15090001u); /* BNE t0,t1,+1 (equal: not taken) */
    put_word(ram, 0x144u, 0x240d002au); /* delay slot */
    put_word(ram, 0x148u, 0x240e002bu);
    psx_cpu_write_gpr(&machine.cpu, 8u, 4u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 4u);
    machine.cpu.pc = 0x140u;
    machine.cpu.next_pc = 0x144u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.delay_slot_pending);
    assert(machine.cpu.delay_branch_pc == 0x140u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(!machine.cpu.delay_slot_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 13u) == 42u);


    /* COP0 transfers: MTC0 Status, MFC0 Status, EPC and RFE. */
    put_word(ram, 0x160u, 0x40886000u); /* MTC0 t0,Status */
    put_word(ram, 0x164u, 0x40096000u); /* MFC0 t1,Status */
    put_word(ram, 0x168u, 0x40887000u); /* MTC0 t0,EPC */
    put_word(ram, 0x16cu, 0x400a7000u); /* MFC0 t2,EPC */
    put_word(ram, 0x170u, 0x42000010u); /* RFE */
    machine.cpu.pc = 0x160u;
    machine.cpu.next_pc = 0x164u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0x0000003cu;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x00000055u);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_status == 0x55u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0x55u);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x55u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x55u);
    machine.cop0_status = 0x0000003cu;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_status == 0x0000000fu);

    /* COP0 is rejected while the current mode is user mode. */
    machine.cop0_status = PSX_COP0_STATUS_KUC;
    machine.cpu.pc = 0x164u;
    machine.cpu.next_pc = 0x168u;
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_COPROCESSOR_UNUSABLE <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));
    machine.cop0_status = 0u;

    /* R3000A load delay: the immediately following instruction reads
     * the old register value; the loaded value is visible afterwards.
     */
    assert(psx_memory_write32(&machine.memory, 0x200u, 0x12345678u));
    assert(psx_memory_write32(&machine.memory, 0x1c0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1c4u, 0x01005021u)); /* ADDU t2,t0,zero */
    assert(psx_memory_write32(&machine.memory, 0x1c8u, 0x01005821u)); /* ADDU t3,t0,zero */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x11111111u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x200u);
    machine.cpu.pc = 0x1c0u; machine.cpu.next_pc = 0x1c4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x11111111u);
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x11111111u);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x12345678u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 0x12345678u);

    /* A write to the same destination in the delay instruction wins. */
    assert(psx_memory_write32(&machine.memory, 0x1d0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1d4u, 0x24080009u)); /* ADDIU t0,zero,9 */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x22222222u);
    machine.cpu.pc = 0x1d0u; machine.cpu.next_pc = 0x1d4u;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 9u);
    assert(!machine.cpu.load_delay_pending);

    /* An unrelated instruction allows the older load to commit. */
    assert(psx_memory_write32(&machine.memory, 0x1e0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1e4u, 0x240c0001u)); /* ADDIU t4,zero,1 */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0u);
    machine.cpu.pc = 0x1e0u; machine.cpu.next_pc = 0x1e4u;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x12345678u);
    assert(psx_cpu_read_gpr(&machine.cpu, 12u) == 1u);

    /* Consecutive loads retire in order and each retain one-instruction delay. */
    assert(psx_memory_write32(&machine.memory, 0x204u, 0x87654321u));
    assert(psx_memory_write32(&machine.memory, 0x1f0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1f4u, 0x8d2a0004u)); /* LW t2,4(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1f8u, 0x240d0002u)); /* ADDIU t5,zero,2 */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 10u, 0u);
    machine.cpu.pc = 0x1f0u; machine.cpu.next_pc = 0x1f4u;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x12345678u);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x87654321u);
    assert(psx_cpu_read_gpr(&machine.cpu, 13u) == 2u);
    machine.cpu.load_delay_pending = false;

    free(bios); free(ram);
    puts("DreamPSX R3000A execution tests: OK");
    return 0;
}
