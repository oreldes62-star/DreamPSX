#include "dreampsx/machine.h"
#include "dreampsx/cop0_irq.h"
#include "dreampsx/cop0.h"

bool psx_machine_init(PsxMachine *machine,
                      uint8_t *ram, size_t ram_size,
                      const uint8_t *bios, size_t bios_size)
{
    if (!machine || !psx_memory_init(&machine->memory, ram, ram_size,
                                     bios, bios_size)) return false;
    psx_machine_reset(machine);
    if (!psx_bus_init(&machine->bus, &machine->memory, &machine->irq))
        return false;
    return true;
}

void psx_machine_reset(PsxMachine *machine)
{
    if (!machine) return;
    psx_irq_reset(&machine->irq);
    psx_cpu_reset(&machine->cpu);
    machine->cop0_status = 0u;
    machine->cop0_cause = 0u;
    machine->cop0_epc = 0u;
    machine->cop0_badvaddr = 0u;
}

void psx_machine_refresh_interrupts(PsxMachine *machine)
{
    if (!machine) return;
    machine->cop0_cause =
        psx_cop0_cause_with_irq_controller(machine->cop0_cause,
                                           &machine->irq);
}

bool psx_machine_interrupt_eligible(const PsxMachine *machine)
{
    if (!machine) return false;
    return psx_cop0_interrupt_eligible(machine->cop0_status,
                                       machine->cop0_cause);
}

void psx_machine_raise_exception(PsxMachine *machine, PsxExceptionCode code,
                                 uint32_t badvaddr, bool in_delay_slot,
                                 uint32_t branch_pc)
{
    uint32_t vector;
    if (!machine) return;

    /* Retire the preceding instruction's pending load before exception
     * entry. The faulting instruction itself is not retired.
     */
    if (machine->cpu.load_delay_pending) {
        psx_cpu_write_gpr(&machine->cpu,
                          machine->cpu.load_delay_register,
                          machine->cpu.load_delay_value);
        machine->cpu.load_delay_pending = false;
        machine->cpu.load_delay_mergeable = false;
    }

    machine->cop0_badvaddr = badvaddr;
    vector = psx_cop0_exception_entry(&machine->cop0_status,
                                      &machine->cop0_cause,
                                      &machine->cop0_epc,
                                      code, machine->cpu.pc,
                                      in_delay_slot, branch_pc);
    machine->cpu.pc = vector;
    machine->cpu.next_pc = vector + 4u;
    machine->cpu.delay_slot_pending = false;
}

/* Execute one instruction with the machine's live COP0 register state. */
bool psx_machine_step(PsxMachine *machine)
{
    if (!machine) return false;
    psx_machine_refresh_interrupts(machine);

    /* Interrupts are sampled at instruction boundaries, never between a
     * branch and its delay-slot instruction. The exception entry redirects
     * execution to the selected general exception vector.
     */
    if (!machine->cpu.delay_slot_pending &&
        psx_machine_interrupt_eligible(machine)) {
        psx_machine_raise_exception(machine, PSX_EXCEPTION_INTERRUPT,
                                     0u, false, 0u);
        return true;
    }

    {
        PsxCpu *cpu = &machine->cpu;
        bool in_delay_slot = cpu->delay_slot_pending;
        uint32_t branch_pc = cpu->delay_branch_pc;
        PsxCpuStepResult result = psx_cpu_step_result_with_cop0(
            cpu, &machine->bus, &machine->cop0_status,
            &machine->cop0_cause, &machine->cop0_epc,
            &machine->cop0_badvaddr);

        if (result == PSX_CPU_STEP_OK) return true;

        switch (result) {
        case PSX_CPU_STEP_FETCH_BUS_ERROR:
            psx_machine_raise_exception(machine, PSX_EXCEPTION_BUS_INSTRUCTION,
                                        cpu->fault_address, in_delay_slot,
                                        branch_pc);
            return true;
        case PSX_CPU_STEP_DATA_BUS_ERROR:
            psx_machine_raise_exception(machine, PSX_EXCEPTION_BUS_DATA,
                                        cpu->fault_address, in_delay_slot,
                                        branch_pc);
            return true;
        case PSX_CPU_STEP_ADDRESS_ERROR_LOAD:
            psx_machine_raise_exception(machine, PSX_EXCEPTION_ADDRESS_LOAD,
                                        cpu->fault_address, in_delay_slot,
                                        branch_pc);
            return true;
        case PSX_CPU_STEP_ADDRESS_ERROR_STORE:
            psx_machine_raise_exception(machine, PSX_EXCEPTION_ADDRESS_STORE,
                                        cpu->fault_address, in_delay_slot,
                                        branch_pc);
            return true;
        case PSX_CPU_STEP_OVERFLOW:
            psx_machine_raise_exception(machine, PSX_EXCEPTION_OVERFLOW,
                                        0u, in_delay_slot, branch_pc);
            return true;
        case PSX_CPU_STEP_COPROCESSOR_UNUSABLE:
            psx_machine_raise_exception(machine,
                                        PSX_EXCEPTION_COPROCESSOR_UNUSABLE,
                                        0u, in_delay_slot, branch_pc);
            return true;
        case PSX_CPU_STEP_SYSCALL:
            psx_machine_raise_exception(machine, PSX_EXCEPTION_SYSCALL,
                                        0u, in_delay_slot, branch_pc);
            return true;
        case PSX_CPU_STEP_BREAKPOINT:
            psx_machine_raise_exception(machine, PSX_EXCEPTION_BREAKPOINT,
                                        0u, in_delay_slot, branch_pc);
            return true;
        case PSX_CPU_STEP_TRAP:
            psx_machine_raise_exception(machine, PSX_EXCEPTION_TRAP,
                                        0u, in_delay_slot, branch_pc);
            return true;
        case PSX_CPU_STEP_RESERVED_INSTRUCTION:
        case PSX_CPU_STEP_UNPREDICTABLE:
            psx_machine_raise_exception(machine,
                                        PSX_EXCEPTION_RESERVED_INSTRUCTION,
                                        0u, in_delay_slot, branch_pc);
            return true;
        case PSX_CPU_STEP_OK:
        default:
            return false;
        }
    }
}
