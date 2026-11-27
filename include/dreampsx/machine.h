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
#include "dreampsx/dma.h"
#include "dreampsx/cdrom.h"

typedef struct {
    PsxIrqController irq;
    PsxMemory memory;
    PsxBus bus;
    PsxCpu cpu;
    PsxGte gte;
    PsxGpu gpu;
    PsxTimers timers;
    PsxDmaController dma;
    PsxCdrom cdrom;
    uint8_t timer2_div8_phase;
    uint32_t cdrom_sector_cycle_phase;
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
/* Advance one GPU scanline, route its HBlank pulse to Timer1, and route
 * VBlank level transitions to IRQ0. */
bool psx_machine_timer_advance_clock(PsxMachine *machine, unsigned timer,
                                     PsxTimerClock clock, uint32_t ticks);
/* Advance the shared system-clock domain. Timer2's /8 source keeps a
 * fractional phase across calls; CPU instruction timing must supply cycles. */
bool psx_machine_advance_system_cycles(PsxMachine *machine, uint32_t cycles);
/* Advance the next CD-ROM sector event after the current sector is drained. */
#define PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES 225792u
bool psx_machine_cdrom_advance_sector(PsxMachine *machine);
bool psx_machine_video_advance_scanline(PsxMachine *machine,
                                        PsxGpuVideoState *state);
bool psx_machine_step(PsxMachine *machine);
void psx_machine_raise_exception(PsxMachine *machine, PsxExceptionCode code,
                                 uint32_t badvaddr, bool in_delay_slot,
                                 uint32_t branch_pc);

#endif
