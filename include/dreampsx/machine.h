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
#include "dreampsx/spu.h"
#include "dreampsx/sio.h"
#include "dreampsx/audio_buffer.h"
#include "dreampsx/boot_config.h"

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
    PsxSpu spu;
    PsxSio sio;
    PsxAudioBuffer audio_output;
    uint8_t timer2_div8_phase;
    uint32_t gpu_scanline_cycle_phase;
    uint32_t gpu_scanline_mode;
    uint32_t timer0_dot_phase;
    uint32_t timer0_dot_divisor;
    uint32_t cdrom_sector_cycle_phase;
    uint32_t spu_audio_cycle_phase;
    uint32_t cop0_status;
    uint32_t cop0_cause;
    uint32_t cop0_epc;
    uint32_t cop0_badvaddr;
} PsxMachine;

/* Load SYSTEM.CNF's PS-X EXE and initialize CPU entry, GP and SP. */
bool psx_machine_boot_from_iso(PsxMachine *machine, const PsxIso9660 *iso,
                               PsxBootConfig *config);
/* Boots from a caller-owned 2048-byte-sector ISO image and mounts the same
 * image as CD-ROM media for subsequent guest reads. */
bool psx_machine_boot_from_disc(PsxMachine *machine, const uint8_t *image,
                                size_t image_size, PsxBootConfig *config);
/* Boots from an already-mounted sector reader (memory-backed or file-backed).
 * The reader context must remain valid until the CD-ROM is unmounted. */
bool psx_machine_boot_from_disc_image(PsxMachine *machine,
                                      const PsxDiscImage *disc,
                                      PsxBootConfig *config);
/* Boots directly from caller-owned CUE text and BIN bytes without expanding
 * raw sectors into a separate ISO buffer. Both inputs must remain alive. */
bool psx_machine_boot_from_cue(PsxMachine *machine, const char *cue_text,
                               size_t cue_size, const uint8_t *bin_data,
                               size_t bin_size, PsxBootConfig *config);
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
bool psx_machine_cdrom_advance_sector(PsxMachine *machine);
bool psx_machine_video_advance_scanline(PsxMachine *machine,
                                        PsxGpuVideoState *state);
bool psx_machine_step(PsxMachine *machine);
/* Render SPU PCM into the bounded machine output queue; read it via read_audio.
 * The shared system clock also schedules 44.1 kHz output automatically.
 */
bool psx_machine_render_audio(PsxMachine *machine, size_t frame_count,
                              size_t *frames_queued);
size_t psx_machine_read_audio(PsxMachine *machine, int16_t *interleaved_stereo,
                              size_t frame_capacity);
void psx_machine_raise_exception(PsxMachine *machine, PsxExceptionCode code,
                                 uint32_t badvaddr, bool in_delay_slot,
                                 uint32_t branch_pc);

#endif
