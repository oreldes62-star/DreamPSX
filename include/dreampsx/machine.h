#ifndef DREAMPSX_MACHINE_H
#define DREAMPSX_MACHINE_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/bus.h"
#include "dreampsx/cop0.h"
#include "dreampsx/cpu.h"
#include "dreampsx/irq.h"
#include "dreampsx/memory.h"

typedef struct {
    PsxIrqController irq;
    PsxMemory memory;
    PsxBus bus;
    PsxCpu cpu;
    uint32_t cop0_status;
    uint32_t cop0_cause;
    uint32_t cop0_epc;
    uint32_t cop0_badvaddr;
} PsxMachine;

bool psx_machine_init(PsxMachine *machine,
                      uint8_t *ram, size_t ram_size,
                      const uint8_t *bios, size_t bios_size);
void psx_machine_reset(PsxMachine *machine);
void psx_machine_refresh_interrupts(PsxMachine *machine);
bool psx_machine_interrupt_eligible(const PsxMachine *machine);
void psx_machine_raise_exception(PsxMachine *machine, PsxExceptionCode code,
                                 uint32_t badvaddr, bool in_delay_slot,
                                 uint32_t branch_pc);

#endif
