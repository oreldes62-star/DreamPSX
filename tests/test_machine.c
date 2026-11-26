#include "dreampsx/machine.h"
#include "dreampsx/cop0_irq.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

int main(void)
{
    PsxMachine machine;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    assert(ram && bios);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE));
    assert(psx_memory_write32(&machine.memory, 0u, 0xdecafbad));
    uint32_t memory_probe = 0u;
    assert(psx_memory_read32(&machine.memory, 0x80000000u, &memory_probe));
    assert(memory_probe == 0xdecafbad);

    /* Reset establishes a clean IRQ/COP0 state. */
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert(psx_irq_read_mask(&machine.irq) == 0u);
    assert(machine.cop0_status == 0u);
    assert(machine.cop0_cause == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    /* Raise a source while masked: Cause.IP2 stays low. */
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* Enable the source and CPU mask/global enable. */
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));

    /* Disable CPU global enable: pending remains, delivery is inhibited. */
    machine.cop0_status &= ~PSX_COP0_STATUS_IEC;
    assert(!psx_machine_interrupt_eligible(&machine));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 2)) != 0u);

    /* Acknowledge source; refresh deasserts IP2. */
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* An eligible hardware IRQ is dispatched automatically at a safe
     * instruction boundary and redirects to the general exception vector.
     */
    machine.cpu.pc = 0x80000000u;
    machine.cpu.next_pc = 0x80000004u;
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x80000000u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_INTERRUPT << PSX_COP0_CAUSE_EXCCODE_SHIFT));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cpu.next_pc == 0x80000084u);
    assert((machine.cop0_status & PSX_COP0_STATUS_IEC) == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));
    /* Clear the latched source before testing unrelated synchronous faults. */
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);
    /* Start the synchronous-fault suite from a clean CPU/COP0/IRQ state. */
    psx_machine_reset(&machine);

    /* Synchronous CPU faults are translated into architectural exceptions. */
    assert(psx_memory_write32(&machine.memory, 0x100u, 0x21280001u)); /* ADDI t0,t1,1 */
    machine.cpu.pc = 0x100u;
    machine.cpu.next_pc = 0x104u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x7fffffffu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x100u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_OVERFLOW << PSX_COP0_CAUSE_EXCCODE_SHIFT));
    assert(machine.cpu.pc == 0x80000080u);

    /* SYSCALL and BREAK use distinct architectural exception codes. */
    assert(psx_memory_write32(&machine.memory, 0x110u, 0x0000000cu));
    machine.cpu.pc = 0x110u;
    machine.cpu.next_pc = 0x114u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x110u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_SYSCALL << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    assert(psx_memory_write32(&machine.memory, 0x114u, 0x0000000du));
    machine.cpu.pc = 0x114u;
    machine.cpu.next_pc = 0x118u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x114u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_BREAKPOINT << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* Invalid fixed fields in an otherwise known SPECIAL opcode are reserved. */
    assert(psx_memory_write32(&machine.memory, 0x118u, 0x00284000u)); /* malformed SLL: rs != 0 */
    machine.cpu.pc = 0x118u;
    machine.cpu.next_pc = 0x11cu;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_RESERVED_INSTRUCTION <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A reserved instruction is no longer a silent boolean failure. */
    assert(psx_memory_write32(&machine.memory, 0x120u, 0xffffffffu));
    machine.cpu.pc = 0x120u;
    machine.cpu.next_pc = 0x124u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x120u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_RESERVED_INSTRUCTION << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A data bus fault records the effective address in BadVAddr. */
    assert(psx_memory_write32(&machine.memory, 0x140u, 0x8d280000u)); /* LW t0,0(t1) */
    machine.cpu.pc = 0x140u;
    machine.cpu.next_pc = 0x144u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x1f000000u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_BUS_DATA << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* Misaligned word loads produce Address Error Load, not Bus Error. */
    machine.cpu.pc = 0x140u;
    machine.cpu.next_pc = 0x144u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x181u);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x181u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_ADDRESS_LOAD << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* COP0 access in user mode produces Coprocessor Unusable. */
    assert(psx_memory_write32(&machine.memory, 0x180u, 0x40096000u)); /* MFC0 t1,Status */
    machine.cpu.pc = 0x180u;
    machine.cpu.next_pc = 0x184u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = PSX_COP0_STATUS_KUC;
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_COPROCESSOR_UNUSABLE << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* Overflow in a branch delay slot must report EPC=branch PC and Cause.BD. */
    assert(psx_memory_write32(&machine.memory, 0x160u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x164u, 0x21280001u)); /* ADDI delay slot */
    machine.cpu.pc = 0x160u;
    machine.cpu.next_pc = 0x164u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x7fffffffu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x160u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_OVERFLOW << PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A REGIMM control transfer in a branch delay slot is rejected and
     * retains the original branch PC in EPC with Cause.BD set.
     */
    assert(psx_memory_write32(&machine.memory, 0x190u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x194u,
                              (0x01u << 26) | (8u << 21) | 1u)); /* BLTZ t0,+1 */
    machine.cpu.pc = 0x190u;
    machine.cpu.next_pc = 0x194u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xffffffffu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x190u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_RESERVED_INSTRUCTION <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A pending load is retired before a synchronous exception enters
     * the handler; the faulting instruction itself is not retired.
     */
    assert(psx_memory_write32(&machine.memory, 0x300u, 0xa5a55a5au));
    assert(psx_memory_write32(&machine.memory, 0x1a0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1a4u, 0x0000000cu)); /* SYSCALL */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x11111111u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x300u);
    machine.cpu.pc = 0x1a0u;
    machine.cpu.next_pc = 0x1a4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x11111111u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xa5a55a5au);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1a4u);

    /* A data-bus fault immediately after a load must retire the older
     * load, while leaving the faulting load's destination untouched.
     */
    assert(psx_memory_write32(&machine.memory, 0x304u, 0x13579bdfu));
    assert(psx_memory_write32(&machine.memory, 0x1b0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1b4u, 0x8d6a0000u)); /* LW t2,0(t3) */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x11111111u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x304u);
    psx_cpu_write_gpr(&machine.cpu, 10u, 0x22222222u);
    psx_cpu_write_gpr(&machine.cpu, 11u, 0x1f000000u);
    machine.cpu.pc = 0x1b0u;
    machine.cpu.next_pc = 0x1b4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x11111111u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x13579bdfu);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0x22222222u);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1b4u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_BUS_DATA <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* A hardware interrupt at the instruction boundary also retires
     * the pending load before entering the exception handler.
     */
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);
    assert(psx_memory_write32(&machine.memory, 0x308u, 0x2468ace0u));
    assert(psx_memory_write32(&machine.memory, 0x1c0u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x1c4u, 0x240c0001u)); /* ADDIU t4,zero,1 */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x33333333u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x308u);
    machine.cpu.pc = 0x1c0u;
    machine.cpu.next_pc = 0x1c4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x33333333u);
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x2468ace0u);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1c4u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_INTERRUPT <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));
    /* Acknowledge the source so later tests start with a clean controller. */
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);

    /* An IRQ must wait until a branch delay-slot load has executed.
     * Once the slot completes, the pending load retires before IRQ entry;
     * EPC then identifies the branch target, not the delay-slot address.
     */
    assert(psx_memory_write32(&machine.memory, 0x30cu, 0xcafebabeu));
    assert(psx_memory_write32(&machine.memory, 0x1d0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x1d4u, 0x8d280000u)); /* LW t0,0(t1), delay slot */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x44444444u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x30cu);
    machine.cpu.pc = 0x1d0u;
    machine.cpu.next_pc = 0x1d4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    assert(psx_machine_step(&machine)); /* IRQ deferred; execute delay slot */
    assert(!machine.cpu.delay_slot_pending);
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x44444444u);
    assert(psx_machine_step(&machine)); /* IRQ now accepted */
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xcafebabeu);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1d8u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_INTERRUPT <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);

    /* A synchronous overflow immediately after a delay-slot LW must
     * retire that load, report EPC at the branch target, and clear Cause.BD.
     */
    assert(psx_memory_write32(&machine.memory, 0x310u, 0xfacec0deu));
    assert(psx_memory_write32(&machine.memory, 0x1e0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_memory_write32(&machine.memory, 0x1e4u, 0x8d480000u)); /* LW t0,0(t2), delay slot */
    assert(psx_memory_write32(&machine.memory, 0x1e8u, 0x21280001u)); /* ADDI t0,t1,1 */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x55555555u);
    psx_cpu_write_gpr(&machine.cpu, 10u, 0x310u);
    machine.cpu.pc = 0x1e0u;
    machine.cpu.next_pc = 0x1e4u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x7fffffffu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine)); /* branch */
    assert(psx_machine_step(&machine)); /* delay-slot load */
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x55555555u);
    assert(psx_machine_step(&machine)); /* target ADDI overflows */
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xfacec0deu);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x1e8u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_OVERFLOW <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* MFC0 is a delayed GPR load: the immediately following ADDIU reads
     * the old destination value, then the COP0 result becomes visible.
     */
    assert(psx_memory_write32(&machine.memory, 0x200u, 0x40086000u)); /* MFC0 t0,Status */
    assert(psx_memory_write32(&machine.memory, 0x204u, 0x25090001u)); /* ADDIU t1,t0,1 */
    machine.cop0_status = 0x00001234u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 5u);
    machine.cpu.pc = 0x200u;
    machine.cpu.next_pc = 0x204u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 5u);
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x1234u);
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 6u);

    /* MTC0 reads the old GPR value when it immediately follows LW. The load
     * retires after the COP0 write, so EPC receives the pre-load value.
     */
    assert(psx_memory_write32(&machine.memory, 0x320u, 0xabcdef01u));
    assert(psx_memory_write32(&machine.memory, 0x210u, 0x8d280000u)); /* LW t0,0(t1) */
    assert(psx_memory_write32(&machine.memory, 0x214u, 0x40887000u)); /* MTC0 t0,EPC */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x11112222u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x320u);
    machine.cpu.pc = 0x210u;
    machine.cpu.next_pc = 0x214u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x11112222u);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xabcdef01u);
    assert(!machine.cpu.load_delay_pending);

    /* An IRQ between MFC0 and its consumer must retire the delayed COP0
     * transfer before exception entry, just like a memory load.
     */
    machine.cop0_epc = 0x13572468u;
    assert(psx_memory_write32(&machine.memory, 0x218u, 0x40087000u)); /* MFC0 t0,EPC */
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xdeadbeefu);
    machine.cpu.pc = 0x218u;
    machine.cpu.next_pc = 0x21cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.load_delay_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xdeadbeefu);
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0x13572468u);
    assert(!machine.cpu.load_delay_pending);
    assert(machine.cop0_epc == 0x21cu);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_INTERRUPT <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));
    psx_irq_set_line(&machine.irq, 2u, false);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_irq_write_mask(&machine.irq, 0u);
    psx_machine_refresh_interrupts(&machine);

    /* Halfword loads/stores and word stores report alignment faults with
     * the original effective address in BadVAddr.
     */
    assert(psx_memory_write32(&machine.memory, 0x340u,
                              (0x21u << 26) | (9u << 21) | (8u << 16)));
    machine.cpu.pc = 0x340u;
    machine.cpu.next_pc = 0x344u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x401u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x401u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_ADDRESS_LOAD <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    assert(psx_memory_write32(&machine.memory, 0x344u,
                              (0x29u << 26) | (9u << 21) | (8u << 16)));
    machine.cpu.pc = 0x344u;
    machine.cpu.next_pc = 0x348u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x401u);
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xbeefu);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x401u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_ADDRESS_STORE <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    assert(psx_memory_write32(&machine.memory, 0x348u,
                              (0x2bu << 26) | (9u << 21) | (8u << 16)));
    machine.cpu.pc = 0x348u;
    machine.cpu.next_pc = 0x34cu;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x402u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x402u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_ADDRESS_STORE <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* An aligned store to an unmapped device region is a Bus Data exception,
     * not an Address Error Store.
     */
    assert(psx_memory_write32(&machine.memory, 0x34cu,
                              (0x2bu << 26) | (9u << 21) | (8u << 16)));
    machine.cpu.pc = 0x34cu;
    machine.cpu.next_pc = 0x350u;
    machine.cpu.delay_slot_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x1f000000u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_BUS_DATA <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* R3000A trap instructions raise exception code 13 only when true. */
    assert(psx_memory_write32(&machine.memory, 0x350u,
                              (8u << 21) | (9u << 16) | 0x34u)); /* TEQ t0,t1 */
    machine.cpu.pc = 0x350u;
    machine.cpu.next_pc = 0x354u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0x12345678u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 0x12345678u);
    machine.cop0_status = 0u;
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_TRAP << PSX_COP0_CAUSE_EXCCODE_SHIFT));
    assert(machine.cop0_epc == 0x350u);

    /* A false TNEI condition does not trap and advances normally. */
    assert(psx_memory_write32(&machine.memory, 0x354u,
                              (0x01u << 26) | (8u << 21) | (14u << 16) | 7u));
    machine.cpu.pc = 0x354u;
    machine.cpu.next_pc = 0x358u;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 7u);
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x358u);

    /* Signed TLTI compares against the sign-extended immediate. */
    assert(psx_memory_write32(&machine.memory, 0x358u,
                              (0x01u << 26) | (8u << 21) | (10u << 16) | 0xffffu));
    machine.cpu.pc = 0x358u;
    machine.cpu.next_pc = 0x35cu;
    machine.cpu.delay_slot_pending = false;
    machine.cop0_status = 0u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xfffffffeu); /* -2 */
    assert(psx_machine_step(&machine));
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_TRAP << PSX_COP0_CAUSE_EXCCODE_SHIFT));
    assert(machine.cop0_epc == 0x358u);

    free(bios);
    free(ram);
    puts("DreamPSX machine-state tests: OK");
    return 0;
}
