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

    /* SWL/SWR partial-word stores: every byte offset, little-endian lanes. */
    {
        static const uint32_t swl_expected[4] = {
            0xaabbcc11u, 0xaabb1122u, 0xaa112233u, 0x11223344u
        };
        static const uint32_t swr_expected[4] = {
            0x11223344u, 0x223344ddu, 0x3344ccddu, 0x44bbccddu
        };
        unsigned offset;
        for (offset = 0u; offset < 4u; ++offset) {
            uint32_t pc_swl = 0x600u + offset * 8u;
            uint32_t pc_swr = 0x640u + offset * 8u;
            uint32_t stored = 0u;
            assert(psx_memory_write32(&machine.memory, 0x3e0u, 0xaabbccddu));
            assert(psx_memory_write32(&machine.memory, pc_swl,
                                      (0x2au << 26) | (9u << 21) |
                                      (8u << 16) | offset));
            assert(psx_memory_write32(&machine.memory, pc_swl + 4u, 0u));
            psx_cpu_write_gpr(&machine.cpu, 8u, 0x11223344u);
            psx_cpu_write_gpr(&machine.cpu, 9u, 0x3e0u);
            machine.cpu.pc = pc_swl; machine.cpu.next_pc = pc_swl + 4u;
            machine.cpu.delay_slot_pending = false;
            machine.cpu.load_delay_pending = false;
            assert(psx_cpu_step(&machine.cpu, &machine.bus));
            assert(psx_memory_read32(&machine.memory, 0x3e0u, &stored));
            assert(stored == swl_expected[offset]);

            assert(psx_memory_write32(&machine.memory, 0x3e0u, 0xaabbccddu));
            assert(psx_memory_write32(&machine.memory, pc_swr,
                                      (0x2eu << 26) | (9u << 21) |
                                      (8u << 16) | offset));
            assert(psx_memory_write32(&machine.memory, pc_swr + 4u, 0u));
            psx_cpu_write_gpr(&machine.cpu, 8u, 0x11223344u);
            psx_cpu_write_gpr(&machine.cpu, 9u, 0x3e0u);
            machine.cpu.pc = pc_swr; machine.cpu.next_pc = pc_swr + 4u;
            machine.cpu.delay_slot_pending = false;
            assert(psx_cpu_step(&machine.cpu, &machine.bus));
            assert(psx_memory_read32(&machine.memory, 0x3e0u, &stored));
            assert(stored == swr_expected[offset]);
        }
    }

    /* SWR/SWL pair stores an unaligned word spanning two aligned words. */
    assert(psx_memory_write32(&machine.memory, 0x3e0u, 0xddccbbaau));
    assert(psx_memory_write32(&machine.memory, 0x3e4u, 0x88776655u));
    assert(psx_memory_write32(&machine.memory, 0x680u,
                              (0x2eu << 26) | (9u << 21) |
                              (8u << 16))); /* SWR t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x684u,
                              (0x2au << 26) | (9u << 21) |
                              (8u << 16) | 3u)); /* SWL t0,3(t1) */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x11223344u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x3e1u);
    machine.cpu.pc = 0x680u; machine.cpu.next_pc = 0x684u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    assert(psx_cpu_step(&machine.cpu, &machine.bus));
    {
        uint32_t first = 0u, second = 0u;
        assert(psx_memory_read32(&machine.memory, 0x3e0u, &first));
        assert(psx_memory_read32(&machine.memory, 0x3e4u, &second));
        assert(first == 0x223344aau);
        assert(second == 0x88776611u);
    }

    /* Variable shifts: low five bits of rs select the shift amount. */
    {
        static const uint32_t amounts[4] = {0u, 1u, 31u, 32u};
        static const uint32_t sll_expected[4] = {
            0x81234567u, 0x02468aceu, 0x80000000u, 0x81234567u
        };
        static const uint32_t srl_expected[4] = {
            0x81234567u, 0x4091a2b3u, 1u, 0x81234567u
        };
        static const uint32_t sra_expected[4] = {
            0x81234567u, 0xc091a2b3u, 0xffffffffu, 0x81234567u
        };
        unsigned i;
        for (i = 0u; i < 4u; ++i) {
            uint32_t pc = 0x700u + i * 12u;
            unsigned funct;
            for (funct = 0u; funct < 3u; ++funct) {
                uint32_t instruction = (9u << 21) | (8u << 16) |
                                       (10u << 11) | (funct == 0u ? 0x04u :
                                       (funct == 1u ? 0x06u : 0x07u));
                assert(psx_memory_write32(&machine.memory, pc + funct * 4u,
                                          instruction));
                psx_cpu_write_gpr(&machine.cpu, 9u, amounts[i]);
                psx_cpu_write_gpr(&machine.cpu, 8u, 0x81234567u);
                machine.cpu.pc = pc + funct * 4u;
                machine.cpu.next_pc = machine.cpu.pc + 4u;
                machine.cpu.delay_slot_pending = false;
                machine.cpu.load_delay_pending = false;
                assert(psx_cpu_step(&machine.cpu, &machine.bus));
                {
                    uint32_t expected_value = (funct == 0u ? sll_expected[i] :
                        (funct == 1u ? srl_expected[i] : sra_expected[i]));
                    uint32_t actual_value = psx_cpu_read_gpr(&machine.cpu, 10u);
                    if (actual_value != expected_value) {
                        fprintf(stderr, "variable shift mismatch: amount=%u kind=%u got=%08x expected=%08x\\n",
                                amounts[i], funct, actual_value, expected_value);
                    }
                    assert(actual_value == expected_value);
                }
            }
        }
    }

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

    /* COP2 transfers: MTC2/MFC2 and CTC2/CFC2. */
    {
        const uint32_t cu2 = 0x40000000u;
        uint32_t value = 0u;
        machine.cop0_status = cu2;
        machine.cpu.delay_slot_pending = false;
        machine.cpu.load_delay_pending = false;
        psx_cpu_write_gpr(&machine.cpu, 8u, 0x00008001u);
        psx_cpu_write_gpr(&machine.cpu, 9u, 0u);
        put_word(ram, 0x500u, (0x12u << 26) | (4u << 21) | (8u << 16) | (9u << 11)); /* MTC2 t0, D9 */
        put_word(ram, 0x504u, (0x12u << 26) | (9u << 16) | (9u << 11)); /* MFC2 t1, D9 */
        put_word(ram, 0x508u, (0x09u << 26) | (9u << 21) | (10u << 16)); /* ADDIU t2,t1,0 */
        put_word(ram, 0x50cu, 0u); /* NOP retires MFC2 result */
        machine.cpu.pc = 0x500u; machine.cpu.next_pc = 0x504u;
        assert(psx_machine_step(&machine));
        assert(psx_gte_read_data(&machine.gte, 9u, &value));
        assert(value == 0xffff8001u);
        assert(psx_machine_step(&machine));
        assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0u); /* MFC2 delay */
        assert(psx_machine_step(&machine));
        assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0u); /* consumer sees old value */
        assert(psx_machine_step(&machine));
        assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0xffff8001u);

        /* Control-register transfers share the same delayed MFC/CFC path. */
        psx_cpu_write_gpr(&machine.cpu, 8u, 0x12345678u);
        psx_cpu_write_gpr(&machine.cpu, 11u, 0u);
        put_word(ram, 0x510u, (0x12u << 26) | (6u << 21) | (8u << 16) | (28u << 11)); /* CTC2 t0,C28 (full-width) */
        put_word(ram, 0x514u, (0x12u << 26) | (2u << 21) | (11u << 16) | (28u << 11)); /* CFC2 t3,C28 */
        put_word(ram, 0x518u, 0u);
        machine.cpu.pc = 0x510u; machine.cpu.next_pc = 0x514u;
        assert(psx_machine_step(&machine));
        assert(psx_machine_step(&machine));
        assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 0u);
        assert(psx_machine_step(&machine));
        assert(psx_cpu_read_gpr(&machine.cpu, 11u) == 0x12345678u);

        /* LWC2/SWC2 move GTE data registers through the main bus. */
        put_word(ram, 0x600u, 0x89abcdefu);
        psx_cpu_write_gpr(&machine.cpu, 12u, 0x600u);
        put_word(ram, 0x520u, (0x32u << 26) | (12u << 21) | (7u << 16)); /* LWC2 D7,0(t4) */
        put_word(ram, 0x524u, (0x3au << 26) | (12u << 21) | (7u << 16) | 4u); /* SWC2 D7,4(t4) */
        machine.cpu.pc = 0x520u; machine.cpu.next_pc = 0x524u;
        assert(psx_machine_step(&machine));
        assert(psx_gte_read_data(&machine.gte, 7u, &value));
        assert(value == 0xcdefu); /* OTZ is unsigned 16-bit. */
        assert(psx_machine_step(&machine));
        assert(psx_memory_read32(&machine.memory, 0x604u, &value));
        assert(value == 0xcdefu);

        /* MFC2's one-instruction delay also applies when the consumer is
         * MTC2: the GTE receives the old GPR value, then the load retires. */
        psx_cpu_write_gpr(&machine.cpu, 9u, 0x11223344u);
        put_word(ram, 0x540u, (0x12u << 26) | (9u << 16) | (9u << 11)); /* MFC2 t1,D9 */
        put_word(ram, 0x544u, (0x12u << 26) | (4u << 21) | (9u << 16) | (0u << 11)); /* MTC2 t1,VXY0 (full-width register) */
        put_word(ram, 0x548u, 0u);
        machine.cpu.pc = 0x540u;
        machine.cpu.next_pc = 0x544u;
        machine.cpu.delay_slot_pending = false;
        machine.cpu.load_delay_pending = false;
        assert(psx_machine_step(&machine));
        assert(psx_machine_step(&machine));
        assert(psx_gte_read_data(&machine.gte, 0u, &value));
        assert(value == 0x11223344u); /* MTC2 saw the pre-MFC2 value. */
        assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0xffff8001u);
        assert(!machine.cpu.load_delay_pending); /* MTC2 retires the pending MFC2 load. */
    }

    /* CTC2/CFC2 preserve the signed 16-bit DQA control-register view. */
    machine.cop0_status = 0x40000000u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x12348001u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0u);
    put_word(ram, 0x54cu, (0x12u << 26) | (6u << 21) | (8u << 16) | (27u << 11)); /* CTC2 t0,C27 */
    put_word(ram, 0x550u, (0x12u << 26) | (2u << 21) | (9u << 16) | (27u << 11)); /* CFC2 t1,C27 */
    put_word(ram, 0x554u, 0u);
    machine.cpu.pc = 0x54cu;
    machine.cpu.next_pc = 0x550u;
    assert(psx_machine_step(&machine));
    assert(machine.gte.control[27u] == 0xffff8001u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0u); /* CFC2 transfer delay */
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0xffff8001u);

    /* CTC2 applies the unsigned H width and FLAG write mask via CPU. */
    machine.cop0_status = 0x40000000u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x12345678u);
    put_word(ram, 0x558u, (0x12u << 26) | (6u << 21) | (8u << 16) | (26u << 11)); /* CTC2 t0,H */
    machine.cpu.pc = 0x558u;
    machine.cpu.next_pc = 0x55cu;
    assert(psx_machine_step(&machine));
    assert(machine.gte.control[26u] == 0x5678u);

    psx_cpu_write_gpr(&machine.cpu, 8u, 0xffffffffu);
    put_word(ram, 0x55cu, (0x12u << 26) | (6u << 21) | (8u << 16) | (31u << 11)); /* CTC2 t0,FLAG */
    machine.cpu.pc = 0x55cu;
    machine.cpu.next_pc = 0x560u;
    assert(psx_machine_step(&machine));
    assert(machine.gte.control[31u] == 0xfffff000u);

    /* MTC2 IRGB and MFC2 ORGB expose the packed color alias. */
    machine.cop0_status = 0x40000000u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x00007fffu);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0u);
    put_word(ram, 0x56cu, (0x12u << 26) | (4u << 21) | (8u << 16) | (28u << 11)); /* MTC2 IRGB */
    put_word(ram, 0x570u, (0x12u << 26) | (9u << 16) | (29u << 11)); /* MFC2 ORGB */
    put_word(ram, 0x574u, 0u);
    machine.cpu.pc = 0x56cu;
    machine.cpu.next_pc = 0x570u;
    assert(psx_machine_step(&machine));
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0x7fffu);

    /* MFC2 LZCR returns the leading-sign count produced by MTC2 LZCS. */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x00ffffffu);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0u);
    put_word(ram, 0x578u, (0x12u << 26) | (4u << 21) | (8u << 16) | (30u << 11)); /* MTC2 LZCS */
    put_word(ram, 0x57cu, (0x12u << 26) | (9u << 16) | (31u << 11)); /* MFC2 LZCR */
    put_word(ram, 0x580u, 0u);
    machine.cpu.pc = 0x578u;
    machine.cpu.next_pc = 0x57cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.gte.data[31u] == 8u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 8u);

    /* LWC2/SWC2 alignment faults must report the effective address. */
    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x601u);
    put_word(ram, 0x550u, (0x32u << 26) | (12u << 21) | (7u << 16)); /* LWC2 D7,0(t4) */
    machine.cpu.pc = 0x550u;
    machine.cpu.next_pc = 0x554u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x601u);
    assert(((machine.cop0_cause >> 2u) & 31u) == PSX_EXCEPTION_ADDRESS_LOAD);

    psx_cpu_write_gpr(&machine.cpu, 12u, 0x603u);
    put_word(ram, 0x554u, (0x3au << 26) | (12u << 21) | (7u << 16)); /* SWC2 D7,0(t4) */
    machine.cpu.pc = 0x554u;
    machine.cpu.next_pc = 0x558u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x603u);
    assert(((machine.cop0_cause >> 2u) & 31u) == PSX_EXCEPTION_ADDRESS_STORE);

    /* CU2 gates memory transfers as well as register transfers/commands. */
    machine.cop0_status = 0u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x600u);
    put_word(ram, 0x558u, (0x32u << 26) | (12u << 21) | (7u << 16)); /* LWC2 D7,0(t4) */
    machine.cpu.pc = 0x558u;
    machine.cpu.next_pc = 0x55cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x558u);
    assert(((machine.cop0_cause >> 2u) & 31u) == PSX_EXCEPTION_COPROCESSOR_UNUSABLE);

    machine.cop0_cause = 0u;
    put_word(ram, 0x55cu, (0x3au << 26) | (12u << 21) | (7u << 16)); /* SWC2 D7,0(t4) */
    machine.cpu.pc = 0x55cu;
    machine.cpu.next_pc = 0x560u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x55cu);
    assert(((machine.cop0_cause >> 2u) & 31u) == PSX_EXCEPTION_COPROCESSOR_UNUSABLE);

    /* MTC2/MFC2 must preserve the SXYP alias semantics through the CPU. */
    uint32_t alias_value = 0u;
    machine.cop0_status = 0x40000000u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x12345678u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0u);
    put_word(ram, 0x560u, (0x12u << 26) | (4u << 21) | (8u << 16) | (15u << 11)); /* MTC2 t0,SXYP */
    put_word(ram, 0x564u, (0x12u << 26) | (9u << 16) | (15u << 11)); /* MFC2 t1,SXYP */
    put_word(ram, 0x568u, 0u);
    machine.cpu.pc = 0x560u;
    machine.cpu.next_pc = 0x564u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(psx_gte_read_data(&machine.gte, 14u, &alias_value));
    assert(alias_value == 0x12345678u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0u); /* MFC2 transfer delay */
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 0x12345678u);

    /* COP2 NCLIP command executes through the CPU command path. */
    machine.cop0_status = 0x40000000u;
    assert(psx_gte_write_data(&machine.gte, 12u, 0x00000000u));
    assert(psx_gte_write_data(&machine.gte, 13u, 0x00000064u));
    assert(psx_gte_write_data(&machine.gte, 14u, 0x00640000u));
    put_word(ram, 0x528u, 0x4b400006u); /* COP2 NCLIP */
    machine.cpu.pc = 0x528u; machine.cpu.next_pc = 0x52cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    {
        uint32_t nclip_value = 0u;
        assert(psx_gte_read_data(&machine.gte, 24u, &nclip_value));
        assert(nclip_value == 10000u);
    }

    /* COP2 must expose NCLIP MAC0 overflow and FLAG through the CPU path. */
    assert(psx_gte_write_data(&machine.gte, 12u, 0x7fff7fffu));
    assert(psx_gte_write_data(&machine.gte, 13u, 0x7fff8000u));
    assert(psx_gte_write_data(&machine.gte, 14u, 0x80008000u));
    put_word(ram, 0x52cu, 0x4b400006u); /* COP2 NCLIP: positive MAC0 overflow */
    machine.cpu.pc = 0x52cu;
    machine.cpu.next_pc = 0x530u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    {
        uint32_t mac0 = 0u;
        uint32_t flags = 0u;
        assert(psx_gte_read_data(&machine.gte, 24u, &mac0));
        assert(mac0 == 0xfffe0001u);
        assert(psx_gte_read_control(&machine.gte, 31u, &flags));
        assert((flags & (1u << 16u)) != 0u);
        assert((flags & (1u << 15u)) == 0u);
        assert((flags & 0x80000000u) != 0u);
    }

    /* COP2 is unavailable when Status.CU2 is clear. */
    machine.cop0_status = 0u;
    machine.cop0_cause = 0u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    put_word(ram, 0x530u, (0x12u << 26) | (9u << 16) | (9u << 11)); /* MFC2 t1,D9 */
    machine.cpu.pc = 0x530u; machine.cpu.next_pc = 0x534u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x530u);
    assert(((machine.cop0_cause >> 2u) & 31u) ==
           PSX_EXCEPTION_COPROCESSOR_UNUSABLE);

    /* COP2 register-transfer encodings require the low 11 bits to be zero. */
    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    put_word(ram, 0x534u, (0x12u << 26) | (9u << 16) | (9u << 11) | 1u);
    machine.cpu.pc = 0x534u;
    machine.cpu.next_pc = 0x538u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x534u);
    assert(((machine.cop0_cause >> 2u) & 31u) ==
           PSX_EXCEPTION_RESERVED_INSTRUCTION);

    /* An unsupported COP2 command must raise Reserved Instruction. */
    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    put_word(ram, 0x534u, 0x4a000002u); /* COP2 command 0x02 is unsupported */
    machine.cpu.pc = 0x534u;
    machine.cpu.next_pc = 0x538u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x534u);
    assert(((machine.cop0_cause >> 2u) & 31u) ==
           PSX_EXCEPTION_RESERVED_INSTRUCTION);

    /* A valid COP2 command also requires Status.CU2. */
    machine.cop0_status = 0u;
    machine.cop0_cause = 0u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    put_word(ram, 0x538u, 0x4b400006u); /* COP2 NCLIP with CU2 disabled */
    machine.cpu.pc = 0x538u;
    machine.cpu.next_pc = 0x53cu;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x538u);
    assert(((machine.cop0_cause >> 2u) & 31u) ==
           PSX_EXCEPTION_COPROCESSOR_UNUSABLE);

    /* CU2 gating takes priority over decoding an unsupported COP2 command. */
    machine.cop0_status = 0u;
    machine.cop0_cause = 0u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    put_word(ram, 0x53cu, 0x4a000002u); /* Unsupported COP2 command, CU2 off */
    machine.cpu.pc = 0x53cu;
    machine.cpu.next_pc = 0x540u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x53cu);
    assert(((machine.cop0_cause >> 2u) & 31u) ==
           PSX_EXCEPTION_COPROCESSOR_UNUSABLE);

    /* A CU2 exception in a branch delay slot records Cause.BD and branch EPC. */
    machine.cop0_status = 0u;
    machine.cop0_cause = 0u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    put_word(ram, 0x540u, 0x10000001u); /* BEQ zero,zero,+1 */
    put_word(ram, 0x544u, 0x4b400006u); /* NCLIP in the delay slot */
    machine.cpu.pc = 0x540u;
    machine.cpu.next_pc = 0x544u;
    assert(psx_machine_step(&machine)); /* branch */
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine)); /* COP2 exception in delay slot */
    assert(machine.cop0_epc == 0x540u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(((machine.cop0_cause >> 2u) & 31u) ==
           PSX_EXCEPTION_COPROCESSOR_UNUSABLE);

    free(bios); free(ram);
    puts("DreamPSX R3000A execution tests: OK");
    return 0;
}
