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
    put_word(ram, 0x10cu, 0x114a0002u); /* BEQ t2,t2,+2: target 0x118 */
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
    /* Branch target is queued as next_pc while 0x110 is the delay slot. */
    assert(machine.cpu.pc == 0x110u && machine.cpu.next_pc == 0x118u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 9u);
    assert(machine.cpu.pc == 0x118u && machine.cpu.next_pc == 0x11cu);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 12u) == 11u);
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 9u);

    psx_cpu_write_gpr(&machine.cpu, 8u, 0x180u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0xfeed1234u);
    psx_cpu_write_gpr(&machine.cpu, 10u, 0u);
    put_word(ram, 0x11cu, 0xad090000u); /* SW t1,0(t0) */
    put_word(ram, 0x120u, 0x8d0a0000u); /* LW t2,0(t0) */
    machine.cpu.pc = 0x11cu;
    machine.cpu.next_pc = 0x120u;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0u); /* load delay */
    assert(psx_cpu_step(&machine.cpu, &machine.bus)); /* retire LW */
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


    /* REGIMM branches: BLTZ/BGEZ, link-on-taken, and delay slots. */
    put_word(ram, 0x400u, (0x01u << 26) | (8u << 21) | (0u << 16) | 2u);  /* BLTZ t0,+2 */
    put_word(ram, 0x404u, 0x240b0001u); /* delay slot */
    put_word(ram, 0x408u, 0x240c0002u); /* skipped */
    put_word(ram, 0x40cu, 0x240d0003u); /* branch target */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xffffffffu);
    machine.cpu.pc = 0x400u; machine.cpu.next_pc = 0x404u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.pc == 0x404u && machine.cpu.next_pc == 0x40cu);
    assert(machine.cpu.delay_slot_pending);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(!machine.cpu.delay_slot_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 1u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 13u) == 3u);

    /* BGEZ not taken still executes its delay slot and continues sequentially. */
    put_word(ram, 0x420u, (0x01u << 26) | (8u << 21) | (1u << 16) | 2u);
    put_word(ram, 0x424u, 0x240e0004u);
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xffffffffu);
    machine.cpu.pc = 0x420u; machine.cpu.next_pc = 0x424u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.delay_slot_pending);
    assert(machine.cpu.next_pc == 0x428u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 14u) == 4u);

    /* BLTZAL writes RA=PC+8 when taken. */
    put_word(ram, 0x440u, (0x01u << 26) | (8u << 21) | (16u << 16) | 1u);
    put_word(ram, 0x444u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x80000000u);
    psx_cpu_write_gpr(&machine.cpu, 31u, 0x11111111u);
    machine.cpu.pc = 0x440u; machine.cpu.next_pc = 0x444u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 31u) == 0x448u);
    assert(machine.cpu.next_pc == 0x448u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));

    /* BGEZAL also links on a taken branch. */
    put_word(ram, 0x450u, (0x01u << 26) | (8u << 21) | (17u << 16) | 1u);
    put_word(ram, 0x454u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 8u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 31u, 0x22222222u);
    machine.cpu.pc = 0x450u; machine.cpu.next_pc = 0x454u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 31u) == 0x458u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));

    put_word(ram, 0x460u, (0x01u << 26) | (8u << 21) | (16u << 16) | 1u);
    put_word(ram, 0x464u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 8u, 1u);
    psx_cpu_write_gpr(&machine.cpu, 31u, 0x33333333u);
    machine.cpu.pc = 0x460u; machine.cpu.next_pc = 0x464u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    /* Link is written even when BLTZAL does not take its branch. */
    assert(psx_cpu_read_gpr(&machine.cpu, 31u) == 0x468u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));

    /* BGEZAL also writes RA when its condition is false. */
    put_word(ram, 0x490u, (0x01u << 26) | (8u << 21) | (17u << 16) | 1u);
    put_word(ram, 0x494u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xffffffffu);
    psx_cpu_write_gpr(&machine.cpu, 31u, 0x44444444u);
    machine.cpu.pc = 0x490u; machine.cpu.next_pc = 0x494u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 31u) == 0x498u);
    assert(machine.cpu.next_pc == 0x498u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));

    /* Link-register aliasing: the branch condition must use the OLD $ra.
     * BLTZAL with negative $ra takes the branch, then replaces $ra with PC+8.
     */
    put_word(ram, 0x4a0u, (0x01u << 26) | (31u << 21) | (16u << 16) | 3u);
    put_word(ram, 0x4a4u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 31u, 0xffffffffu);
    machine.cpu.pc = 0x4a0u; machine.cpu.next_pc = 0x4a4u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.next_pc == 0x4b0u); /* taken using old negative $ra */
    assert(psx_cpu_read_gpr(&machine.cpu, 31u) == 0x4a8u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));

    /* BGEZAL with negative $ra is NOT taken, but still links.
     * A premature link write would incorrectly make the branch taken.
     */
    put_word(ram, 0x4c0u, (0x01u << 26) | (31u << 21) | (17u << 16) | 3u);
    put_word(ram, 0x4c4u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 31u, 0xffffffffu);
    machine.cpu.pc = 0x4c0u; machine.cpu.next_pc = 0x4c4u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.next_pc == 0x4c8u); /* not taken using old negative $ra */
    assert(psx_cpu_read_gpr(&machine.cpu, 31u) == 0x4c8u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));

    /* Unsupported REGIMM sub-op encodings are reserved instructions. */
    put_word(ram, 0x460u, (0x01u << 26) | (8u << 21) | (2u << 16));
    machine.cpu.pc = 0x460u; machine.cpu.next_pc = 0x464u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step_result_with_cop0(
               &machine.cpu, &machine.bus, &machine.cop0_status,
               &machine.cop0_cause, &machine.cop0_epc,
               &machine.cop0_badvaddr) ==
           PSX_CPU_STEP_RESERVED_INSTRUCTION);

    /* BLEZ and BGTZ require the encoded rt field to be zero. */
    put_word(ram, 0x468u, (0x06u << 26) | (8u << 21) | (1u << 16));
    machine.cpu.pc = 0x468u; machine.cpu.next_pc = 0x46cu;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step_result_with_cop0(
               &machine.cpu, &machine.bus, &machine.cop0_status,
               &machine.cop0_cause, &machine.cop0_epc,
               &machine.cop0_badvaddr) ==
           PSX_CPU_STEP_RESERVED_INSTRUCTION);

    put_word(ram, 0x46cu, (0x07u << 26) | (8u << 21) | (2u << 16));
    machine.cpu.pc = 0x46cu; machine.cpu.next_pc = 0x470u;
    machine.cpu.delay_slot_pending = false;
    assert(psx_cpu_step_result_with_cop0(
               &machine.cpu, &machine.bus, &machine.cop0_status,
               &machine.cop0_cause, &machine.cop0_epc,
               &machine.cop0_badvaddr) ==
           PSX_CPU_STEP_RESERVED_INSTRUCTION);

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
    psx_cpu_write_gpr(&machine.cpu, 9u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 10u, 0u);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_status == 0x55u);
    assert(psx_machine_step(&machine)); /* MFC0 queues t1 */
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0u);
    assert(psx_machine_step(&machine)); /* MTC0 retires t1 after reading t0 */
    assert(machine.cop0_epc == 0x55u);
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0x55u);
    assert(psx_machine_step(&machine)); /* MFC0 queues t2 */
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0u);
    machine.cop0_status = 0x0000003cu;
    assert(psx_machine_step(&machine)); /* RFE; t2 retires */
    assert(machine.cop0_status == 0x0000003fu);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x55u);

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

    /* A branch immediately after LW must compare the OLD destination value.
     * The load retires after the branch has read its operands, so BEQ still
     * takes its path even though the loaded value is nonzero.
     */
    assert(psx_memory_write32(&machine.memory, 0x208u, 1u));
    assert(psx_memory_write32(&machine.memory, 0x1f0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1f4u, 0x11000001u)); /* BEQ t0,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x1f8u, 0x240a002au)); /* delay slot */
    assert(psx_memory_write32(&machine.memory, 0x1fcu, 0x240b002bu)); /* branch target */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x208u);
    machine.cpu.pc = 0x1f0u; machine.cpu.next_pc = 0x1f4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.next_pc == 0x1fcu); /* branch used the old zero value */
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 1u); /* load retired after compare */
    assert(machine.cpu.delay_slot_pending);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 42u);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 43u);

    /* Consecutive loads retire in order and each retain one-instruction delay. */
    assert(psx_memory_write32(&machine.memory, 0x204u, 0x87654321u));
    assert(psx_memory_write32(&machine.memory, 0x1f0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1f4u, 0x8d2a0004u)); /* LW t2,4(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1f8u, 0x240d0002u)); /* ADDIU t5,zero,2 */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x200u);
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

    /* A delayed load to $ra followed by JALR $ra,$ra must use the
     * old $ra as the jump target. JALR's link write then wins over the
     * pending load, rather than allowing the loaded target to replace it.
     */
    assert(psx_memory_write32(&machine.memory, 0x700u, 0x560u));
    assert(psx_memory_write32(&machine.memory, 0x500u, 0x8d3f0000u)); /* LW ra,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x504u,
                              (31u << 21) | (31u << 11) | 0x09u)); /* JALR ra,ra */
    assert(psx_memory_write32(&machine.memory, 0x508u, 0x24080001u)); /* delay slot */
    assert(psx_memory_write32(&machine.memory, 0x540u, 0x240a002au)); /* old target */
    assert(psx_memory_write32(&machine.memory, 0x560u, 0x240a0063u)); /* loaded target */
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x700u);
    psx_cpu_write_gpr(&machine.cpu, 31u, 0x540u);
    machine.cpu.pc = 0x500u; machine.cpu.next_pc = 0x504u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.next_pc == 0x540u);
    assert(psx_cpu_read_gpr(&machine.cpu, 31u) == 0x50cu);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cpu.delay_slot_pending);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 42u);

    /* A delayed LW to $ra followed by BGEZAL $ra must test the OLD
     * negative $ra, remain not-taken, and still write PC+8 to $ra.
     */
    assert(psx_memory_write32(&machine.memory, 0x704u, 0x7fffffffu));
    assert(psx_memory_write32(&machine.memory, 0x580u, 0x8d3f0000u)); /* LW ra,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x584u,
                              (0x01u << 26) | (31u << 21) | (17u << 16) | 1u));
    assert(psx_memory_write32(&machine.memory, 0x588u, 0x24080001u)); /* delay slot */
    assert(psx_memory_write32(&machine.memory, 0x58cu, 0x240a002au)); /* fall-through */
    assert(psx_memory_write32(&machine.memory, 0x590u, 0x240b002bu)); /* branch target */
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x704u);
    psx_cpu_write_gpr(&machine.cpu, 31u, 0xffffffffu);
    machine.cpu.pc = 0x580u; machine.cpu.next_pc = 0x584u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.next_pc == 0x58cu); /* old negative $ra: not taken */
    assert(psx_cpu_read_gpr(&machine.cpu, 31u) == 0x58cu); /* link wins */
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cpu.delay_slot_pending);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 42u);

    /* LWL/LWR merge semantics on little-endian memory, all byte offsets. */
    assert(psx_memory_write32(&machine.memory, 0x220u, 0x44332211u));
    assert(psx_memory_write32(&machine.memory, 0x224u, 0x88776655u));
    {
        static const uint32_t lwl_expected[4] = {
            0x11bbccddu, 0x2211ccddu, 0x332211ddu, 0x44332211u
        };
        static const uint32_t lwr_expected[4] = {
            0x44332211u, 0xaa443322u, 0xaabb4433u, 0xaabbcc44u
        };
        unsigned offset;
        for (offset = 0u; offset < 4u; ++offset) {
            uint32_t pc_lwl = 0x240u + offset * 8u;
            uint32_t pc_lwr = 0x280u + offset * 8u;
            assert(psx_memory_write32(&machine.memory, pc_lwl,
                                      (0x22u << 26) | (9u << 21) |
                                      (8u << 16) | offset));
            assert(psx_memory_write32(&machine.memory, pc_lwl + 4u, 0u));
            psx_cpu_write_gpr(&machine.cpu, 8u, 0xaabbccddu);
            psx_cpu_write_gpr(&machine.cpu, 9u, 0x220u);
            machine.cpu.pc = pc_lwl; machine.cpu.next_pc = pc_lwl + 4u;
            machine.cpu.load_delay_pending = false;
            assert(psx_cpu_step(&machine.cpu, &machine.bus));
            assert(machine.cpu.load_delay_pending);
            assert(psx_cpu_step(&machine.cpu, &machine.bus));
            assert(psx_cpu_read_gpr(&machine.cpu, 8u) == lwl_expected[offset]);
            machine.cpu.load_delay_pending = false;

            assert(psx_memory_write32(&machine.memory, pc_lwr,
                                      (0x26u << 26) | (9u << 21) |
                                      (8u << 16) | offset));
            assert(psx_memory_write32(&machine.memory, pc_lwr + 4u, 0u));
            psx_cpu_write_gpr(&machine.cpu, 8u, 0xaabbccddu);
            machine.cpu.pc = pc_lwr; machine.cpu.next_pc = pc_lwr + 4u;
            machine.cpu.load_delay_pending = false;
            assert(psx_cpu_step(&machine.cpu, &machine.bus));
            assert(psx_cpu_step(&machine.cpu, &machine.bus));
            assert(psx_cpu_read_gpr(&machine.cpu, 8u) == lwr_expected[offset]);
            machine.cpu.load_delay_pending = false;
        }
    }

    /* Adjacent LWL/LWR to the same rt forward the first partial word. */
    assert(psx_memory_write32(&machine.memory, 0x300u,
                              (0x22u << 26) | (9u << 21) | (8u << 16) | 3u));
    assert(psx_memory_write32(&machine.memory, 0x304u,
                              (0x26u << 26) | (9u << 21) | (8u << 16)));
    assert(psx_memory_write32(&machine.memory, 0x308u, 0u));
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xdeadbeefu);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x221u);
    machine.cpu.pc = 0x300u; machine.cpu.next_pc = 0x304u;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x55adbeefu);
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x55443322u);
    machine.cpu.load_delay_pending = false;

    /* Signed/unsigned byte and halfword loads, plus little-endian stores. */
    assert(psx_memory_write8(&machine.memory, 0x3c0u, 0x80u));
    assert(psx_memory_write16(&machine.memory, 0x3c2u, 0x8001u));
    put_word(ram, 0x360u, (0x20u << 26) | (9u << 21) | (8u << 16)); /* LB */
    put_word(ram, 0x364u, 0u);
    put_word(ram, 0x368u, (0x24u << 26) | (9u << 21) | (8u << 16)); /* LBU */
    put_word(ram, 0x36cu, 0u);
    put_word(ram, 0x370u, (0x21u << 26) | (9u << 21) | (8u << 16)); /* LH */
    put_word(ram, 0x374u, 0u);
    put_word(ram, 0x378u, (0x25u << 26) | (9u << 21) | (8u << 16)); /* LHU */
    put_word(ram, 0x37cu, 0u);

    psx_cpu_write_gpr(&machine.cpu, 9u, 0x3c0u);
    machine.cpu.pc = 0x360u; machine.cpu.next_pc = 0x364u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xffffff80u);

    psx_cpu_write_gpr(&machine.cpu, 9u, 0x3c0u);
    machine.cpu.pc = 0x368u; machine.cpu.next_pc = 0x36cu;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x80u);

    psx_cpu_write_gpr(&machine.cpu, 9u, 0x3c2u);
    machine.cpu.pc = 0x370u; machine.cpu.next_pc = 0x374u;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xffff8001u);

    psx_cpu_write_gpr(&machine.cpu, 9u, 0x3c2u);
    machine.cpu.pc = 0x378u; machine.cpu.next_pc = 0x37cu;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x8001u);

    put_word(ram, 0x380u, (0x28u << 26) | (9u << 21) | (8u << 16)); /* SB */
    put_word(ram, 0x384u, (0x29u << 26) | (9u << 21) | (8u << 16)); /* SH */
    put_word(ram, 0x388u, (0x2bu << 26) | (9u << 21) | (8u << 16)); /* SW */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xa1b2c3d4u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x3d0u);
    machine.cpu.pc = 0x380u; machine.cpu.next_pc = 0x384u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    {
        uint8_t stored_byte = 0u;
        uint16_t stored_half = 0u;
        uint32_t stored_word = 0u;
        assert(psx_memory_read8(&machine.memory, 0x3d0u, &stored_byte));
        assert(psx_memory_read16(&machine.memory, 0x3d0u, &stored_half));
        assert(psx_memory_read32(&machine.memory, 0x3d0u, &stored_word));
        assert(stored_byte == 0xd4u);
        assert(stored_half == 0xc3d4u);
        assert(stored_word == 0xa1b2c3d4u);
    }

    free(bios); free(ram);
    puts("DreamPSX R3000A execution tests: OK");
    return 0;
}
