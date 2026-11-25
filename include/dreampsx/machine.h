#ifndef DREAMPSX_MACHINE_H
#define DREAMPSX_MACHINE_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/bus.h"
#include "dreampsx/irq.h"
#include "dreampsx/memory.h"

/* Shared machine-level state for implemented components.
 * CPU registers and remaining peripherals will be attached as their interfaces land.
 */
typedef struct {
    PsxIrqController irq;
    PsxMemory memory;
    PsxBus bus;
    uint32_t cop0_status;
    uint32_t cop0_cause;
} PsxMachine;

bool psx_machine_init(PsxMachine *machine,
                      uint8_t *ram, size_t ram_size,
                      const uint8_t *bios, size_t bios_size);
void psx_machine_reset(PsxMachine *machine);
void psx_machine_refresh_interrupts(PsxMachine *machine);
bool psx_machine_interrupt_eligible(const PsxMachine *machine);

#endif
