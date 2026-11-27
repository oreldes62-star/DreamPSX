#ifndef DREAMPSX_MACHINE_H
#define DREAMPSX_MACHINE_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/bus.h"
#include "dreampsx/cop0.h"
#include "dreampsx/cpu.h"
#include "dreampsx/irq.h"
#include "dreampsx/gte.h"
#include "dreampsx/gpu.h"
#include "dreampsx/memory.h"
#include "dreampsx/timers.h"

typedef struct {
    PsxIrqController irq;
    PsxMemory memory;
    PsxBus bus;
    PsxCpu cpu;
    PsxGte gte;
    PsxGpu gpu;
    PsxTimers timers;
    uint32_t cop0_status;
    uint32_t cop0_cause;
    uint32_t cop0_epc;
    uint32_t cop0_badvaddr;
} PsxMachine;

bool psx_machine_init(PsxMachine *machine,
                      uint8_t *ram, size_t ram_size,
                      const uint8_t *bios, size_t bios_size,
                      uint16_t *gpu_vram, size_t gpu_vram_pixels);
void psx_machine_reset(PsxMachine *machine);
void psx_machine_refresh_interrupts(PsxMachine *machine);
bool psx_machine_interrupt_eligible(const PsxMachine *machine);
/* Advance one GPU scanline and route VBlank transitions to IRQ0. */
bool psx_machine_video_advance_scanline(PsxMachine *machine,
                                        PsxGpuVideoState *state);
bool psx_machine_step(PsxMachine *machine);
void psx_machine_raise_exception(PsxMachine *machine, PsxExceptionCode code,
                                 uint32_t badvaddr, bool in_delay_slot,
                                 uint32_t branch_pc);

#endif
