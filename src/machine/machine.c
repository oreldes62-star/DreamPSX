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

    machine->cop0_badvaddr = badvaddr;
    vector = psx_cop0_exception_entry(&machine->cop0_status,
                                      &machine->cop0_cause,
                                      &machine->cop0_epc,
                                      code, machine->cpu.pc,
                                      in_delay_slot, branch_pc);
    machine->cpu.pc = vector;
    machine->cpu.next_pc = vector + 4u;
}

/* Execute one instruction with the machine's live COP0 register state. */
bool psx_machine_step(PsxMachine *machine)
{
    if (!machine) return false;
    psx_machine_refresh_interrupts(machine);
    return psx_cpu_step_with_cop0(&machine->cpu, &machine->bus,
                                  &machine->cop0_status,
                                  &machine->cop0_cause,
                                  &machine->cop0_epc,
                                  &machine->cop0_badvaddr);
}
