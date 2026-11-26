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

    free(bios);
    free(ram);
    puts("DreamPSX machine-state tests: OK");
    return 0;
}
