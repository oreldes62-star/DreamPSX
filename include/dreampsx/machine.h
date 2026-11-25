#ifndef DREAMPSX_MACHINE_H
#define DREAMPSX_MACHINE_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/irq.h"

/* Shared machine-level state for components that have been implemented.
 * CPU registers and RAM will be attached here as their core interfaces land.
 */
typedef struct {
    PsxIrqController irq;
    uint32_t cop0_status;
    uint32_t cop0_cause;
} PsxMachine;

void psx_machine_reset(PsxMachine *machine);
void psx_machine_refresh_interrupts(PsxMachine *machine);
bool psx_machine_interrupt_eligible(const PsxMachine *machine);

#endif
