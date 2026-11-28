#include "dreampsx/machine.h"
#include "dreampsx/cop0_irq.h"
#include "dreampsx/cop0.h"

bool psx_machine_boot_from_iso(PsxMachine *machine, const PsxIso9660 *iso,
                               PsxBootConfig *config)
{
    PsxBootConfig parsed_config;
    PsxExeInfo exe_info;
    if (!machine || !machine->memory.initialized ||
        !psx_boot_load_from_iso(&machine->memory, iso, &parsed_config,
                                &exe_info))
        return false;

    psx_cpu_reset(&machine->cpu);
    psx_cpu_write_gpr(&machine->cpu, 28u, exe_info.global_pointer);
    psx_cpu_write_gpr(&machine->cpu, 29u, exe_info.initial_stack_pointer);
    machine->cpu.pc = exe_info.entry_pc;
    machine->cpu.next_pc = exe_info.entry_pc + 4u;
    machine->cpu.delay_slot_pending = false;
    machine->cpu.load_delay_pending = false;
    machine->cpu.load_delay_mergeable = false;
    if (config)
        *config = parsed_config;
    return true;
}

bool psx_machine_boot_from_disc(PsxMachine *machine, const uint8_t *image,
                                size_t image_size, PsxBootConfig *config)
{
    PsxIso9660 iso;
    if (!machine || !image ||
        !psx_iso9660_mount(&iso, image, image_size) ||
        !psx_cdrom_mount_iso(&machine->cdrom, image, image_size))
        return false;
    if (!psx_machine_boot_from_iso(machine, &iso, config)) {
        psx_cdrom_unmount(&machine->cdrom);
        psx_irq_set_line(&machine->irq, 2u,
                         psx_cdrom_irq_output(&machine->cdrom));
        psx_machine_refresh_interrupts(machine);
        return false;
    }
    return true;
}

bool psx_machine_boot_from_disc_image(PsxMachine *machine,
                                      const PsxDiscImage *disc,
                                      PsxBootConfig *config)
{
    PsxIso9660 iso;
    if (!machine || !machine->memory.initialized ||
        !machine->cdrom.initialized ||
        !psx_iso9660_mount_disc(&iso, disc) ||
        !psx_cdrom_mount_disc(&machine->cdrom, disc))
        return false;
    if (!psx_machine_boot_from_iso(machine, &iso, config)) {
        psx_cdrom_unmount(&machine->cdrom);
        psx_irq_set_line(&machine->irq, 2u,
                         psx_cdrom_irq_output(&machine->cdrom));
        psx_machine_refresh_interrupts(machine);
        return false;
    }
    return true;
}

bool psx_machine_boot_from_cue(PsxMachine *machine, const char *cue_text,
                               size_t cue_size, const uint8_t *bin_data,
                               size_t bin_size, PsxBootConfig *config)
{
    PsxDiscImage disc;
    if (!machine ||
        !psx_disc_mount_cue(&disc, cue_text, cue_size, bin_data, bin_size))
        return false;
    return psx_machine_boot_from_disc_image(machine, &disc, config);
}

bool psx_machine_init(PsxMachine *machine,
                      uint8_t *ram, size_t ram_size,
                      const uint8_t *bios, size_t bios_size,
                      uint16_t *gpu_vram, size_t gpu_vram_pixels)
{
    if (!machine || !psx_memory_init(&machine->memory, ram, ram_size,
                                     bios, bios_size)) return false;
    if (!psx_gpu_init(&machine->gpu, gpu_vram, gpu_vram_pixels)) return false;
    if (!psx_dma_init(&machine->dma)) return false;
    if (!psx_cdrom_init(&machine->cdrom)) return false;
    if (!psx_spu_init(&machine->spu)) return false;
    if (!psx_sio_init(&machine->sio)) return false;
    if (!psx_audio_buffer_init(&machine->audio_output)) return false;
    psx_machine_reset(machine);
    if (!psx_bus_init(&machine->bus, &machine->memory, &machine->irq,
                     &machine->gpu, &machine->timers))
        return false;
    psx_bus_attach_dma(&machine->bus, &machine->dma);
    psx_bus_attach_cdrom(&machine->bus, &machine->cdrom);
    machine->bus.spu = &machine->spu;
    psx_bus_attach_sio(&machine->bus, &machine->sio);
    psx_sio_attach_irq(&machine->sio, &machine->irq);
    return true;
}

void psx_machine_reset(PsxMachine *machine)
{
    if (!machine) return;
    psx_irq_reset(&machine->irq);
    psx_cpu_reset(&machine->cpu);
    psx_gte_reset(&machine->gte);
    psx_gpu_reset(&machine->gpu);
    psx_timers_reset(&machine->timers);
    psx_dma_reset(&machine->dma);
    psx_cdrom_reset(&machine->cdrom);
    psx_spu_reset(&machine->spu);
    psx_sio_reset(&machine->sio);
    psx_audio_buffer_reset(&machine->audio_output);
    machine->timer2_div8_phase = 0u;
    machine->gpu_scanline_cycle_phase = 0u;
    machine->gpu_scanline_mode = machine->gpu.display.mode;
    machine->cdrom_sector_cycle_phase = 0u;
    machine->spu_audio_cycle_phase = 0u;
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


bool psx_machine_cdrom_advance_sector(PsxMachine *machine)
{
    bool advanced;
    if (!machine) return false;
    advanced = psx_cdrom_advance_sector(&machine->cdrom);
    /* Even an end-of-image event can raise INT5, so always refresh IRQ2. */
    psx_irq_set_line(&machine->irq, 2u,
                     psx_cdrom_irq_output(&machine->cdrom));
    psx_machine_refresh_interrupts(machine);
    return advanced;
}

/* Timer IRQs map to interrupt controller sources 4, 5 and 6. */
bool psx_machine_timer_advance_clock(PsxMachine *machine, unsigned timer,
                                     PsxTimerClock clock, uint32_t ticks)
{
    uint8_t irq_sources = 0u;
    if (!machine || !psx_timers_advance_clock(&machine->timers, timer,
                                               clock, ticks, &irq_sources))
        return false;
    if ((irq_sources & (1u << timer)) != 0u) {
        psx_irq_set_line(&machine->irq, 4u + timer, true);
        psx_irq_set_line(&machine->irq, 4u + timer, false);
        psx_machine_refresh_interrupts(machine);
    }
    return true;
}

/* Feed elapsed system-clock cycles into the three root counters.
 * Timer2's divide-by-eight clock is modeled with a persistent remainder so
 * splitting the same interval across calls cannot change the result.
 */
bool psx_machine_advance_system_cycles(PsxMachine *machine, uint32_t cycles)
{
    if (!machine) return false;

    /* The GPU raster shares the CPU master-clock domain. These integer
     * line periods are a first-order PAL/NTSC approximation; horizontal
     * dot timing and half-line interlace phase remain future refinements.
     * Retaining the remainder makes cycle batching deterministic.
     */
    {
        PsxGpuDisplayModeInfo mode;
        const bool mode_valid =
            psx_gpu_get_display_mode_info(&machine->gpu, &mode);
        const uint32_t current_mode = machine->gpu.display.mode;
        const uint32_t line_cycles = mode_valid && mode.pal ? 2157u : 2148u;

        /* GP1 display-mode changes can switch PAL/NTSC or progressive/
         * interlaced cadence. Do not carry a remainder measured in the old
         * mode into the new raster period.
         */
        if (machine->gpu_scanline_mode != current_mode) {
            machine->gpu_scanline_cycle_phase = 0u;
            machine->gpu_scanline_mode = current_mode;
        }
        uint64_t phase = (uint64_t)machine->gpu_scanline_cycle_phase +
                         (uint64_t)cycles;
        while (phase >= line_cycles) {
            PsxGpuVideoState video;
            phase -= line_cycles;
            if (!psx_machine_video_advance_scanline(machine, &video))
                return false;
        }
        machine->gpu_scanline_cycle_phase = (uint32_t)phase;
    }

    /* SIO serial transfers share the PS1 master-clock domain. */
    psx_sio_advance_cycles(&machine->sio, cycles);

    for (unsigned timer = 0u; timer < 3u; ++timer) {
        if (!psx_machine_timer_advance_clock(machine, timer,
                                              PSX_TIMER_CLOCK_SYSTEM,
                                              cycles))
            return false;
    }

    const uint64_t accumulated =
        (uint64_t)machine->timer2_div8_phase + (uint64_t)cycles;
    const uint32_t divided_ticks = (uint32_t)(accumulated / 8u);
    machine->timer2_div8_phase = (uint8_t)(accumulated % 8u);
    if (divided_ticks != 0u &&
        !psx_machine_timer_advance_clock(machine, 2u,
                                         PSX_TIMER_CLOCK_SYSTEM_DIV8,
                                         divided_ticks))
        return false;

    /* CD-ROM command completion and sector cadence share the PS1 clock. */
    /* The PS1 master clock is 33.8688 MHz; the SPU output cadence is
     * 44.1 kHz (one stereo frame per 768 master-clock cycles). Preserve
     * fractional phase across calls so instruction-sized chunks do not drift.
     */
    {
        const uint64_t audio_cycles =
            (uint64_t)machine->spu_audio_cycle_phase + (uint64_t)cycles;
        const size_t due_frames = (size_t)(audio_cycles / 768u);
        machine->spu_audio_cycle_phase = (uint32_t)(audio_cycles % 768u);
        if (due_frames != 0u) {
            size_t queued = 0u;
            if (!psx_machine_render_audio(machine, due_frames, &queued))
                return false;
        }
    }

    psx_cdrom_advance_cycles(&machine->cdrom, cycles);
    psx_cdrom_advance_xa_audio_cycles(&machine->cdrom, cycles);
    psx_irq_set_line(&machine->irq, 2u,
                     psx_cdrom_irq_output(&machine->cdrom));
    psx_machine_refresh_interrupts(machine);

    if (machine->cdrom.read_active) {
        const uint32_t sector_period =
            psx_cdrom_sector_period_cycles(&machine->cdrom);
        uint64_t phase = (uint64_t)machine->cdrom_sector_cycle_phase +
                         (uint64_t)cycles;
        if (sector_period != 0u && phase >= sector_period)
            phase = sector_period;
        machine->cdrom_sector_cycle_phase = (uint32_t)phase;
        if (sector_period != 0u &&
            machine->cdrom_sector_cycle_phase >= sector_period &&
            machine->cdrom.data_count == 0u) {
            if (psx_machine_cdrom_advance_sector(machine))
                machine->cdrom_sector_cycle_phase = 0u;
            else if (!machine->cdrom.read_active)
                machine->cdrom_sector_cycle_phase = 0u;
        }
    } else {
        machine->cdrom_sector_cycle_phase = 0u;
    }
    return true;
}

/* The PS1 interrupt controller maps VBlank to source 0 (IRQ0). Keep the
 * source level asserted for the complete blanking interval; the controller
 * latches the rising edge and software acknowledges it through I_STAT.
 */
bool psx_machine_video_advance_scanline(PsxMachine *machine,
                                        PsxGpuVideoState *state)
{
    if (!machine || !state ||
        !psx_gpu_video_advance_scanline(&machine->gpu, state))
        return false;

    /* Timer 1 synchronization follows the GPU's VBlank level. The
     * raster scheduler updates it once per scanline; exact vertical-edge
     * sub-line timing remains a future refinement.
     */
    if (!psx_timers_set_sync_signal(&machine->timers, 1u, state->vblank))
        return false;

    /* Timer 0 synchronization observes the HBlank edge. The current GPU
     * API reports a line event rather than a timed horizontal interval, so
     * this pulse currently supports edge-triggered modes (1 and 3). Modes
     * that gate clocks throughout blanking need the future dot-phase model.
     */
    if (state->hblank_pulse) {
        if (!psx_timers_set_sync_signal(&machine->timers, 0u, true) ||
            !psx_timers_set_sync_signal(&machine->timers, 0u, false))
            return false;
    }

    /* Timer 1 receives one abstract HBlank clock tick per completed line.
     * Its VBlank synchronization gate is applied by the timer subsystem.
     */
    if (state->hblank_pulse &&
        !psx_machine_timer_advance_clock(machine, 1u,
                                         PSX_TIMER_CLOCK_HBLANK, 1u))
        return false;

    psx_irq_set_line(&machine->irq, 0u, state->vblank);
    psx_machine_refresh_interrupts(machine);
    return true;
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
        PsxCpuStepResult result = psx_cpu_step_result_with_cop0_gte(
            cpu, &machine->bus, &machine->cop0_status,
            &machine->cop0_cause, &machine->cop0_epc,
            &machine->cop0_badvaddr, &machine->gte);

        /* Feed the CPU's current baseline timing into the shared hardware
         * clock. Timer IRQs are therefore visible at the next instruction
         * boundary, not in the middle of the instruction just executed. */
        if (!psx_machine_advance_system_cycles(
                machine, psx_cpu_last_step_cycles(cpu)))
            return false;

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
            /* This exception does not report a bad virtual address. Preserve
             * the previous BadVAddr value instead of replacing it with zero. */
            psx_machine_raise_exception(machine,
                                        PSX_EXCEPTION_COPROCESSOR_UNUSABLE,
                                        machine->cop0_badvaddr,
                                        in_delay_slot, branch_pc);
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

/* Render in small stack blocks so the 16 MB Dreamcast target does not need
 * a large temporary PCM allocation. Audio timing is supplied by the caller.
 */
bool psx_machine_render_audio(PsxMachine *machine, size_t frame_count,
                              size_t *frames_queued)
{
    int16_t block[128u * PSX_AUDIO_CHANNEL_COUNT];
    size_t queued = 0u;
    if (frames_queued) *frames_queued = 0u;
    if (!machine || !machine->spu.initialized ||
        !machine->audio_output.initialized || !frames_queued)
        return false;

    while (frame_count != 0u) {
        const size_t chunk = frame_count > 128u ? 128u : frame_count;
        size_t rendered = 0u;
        if (!psx_spu_mix_block(&machine->spu, block, chunk, &rendered) ||
            rendered != chunk)
            return false;
        queued += psx_audio_buffer_write(&machine->audio_output, block,
                                         rendered);
        frame_count -= rendered;
    }
    *frames_queued = queued;
    return true;
}

size_t psx_machine_read_audio(PsxMachine *machine, int16_t *interleaved_stereo,
                              size_t frame_capacity)
{
    if (!machine || !machine->audio_output.initialized) return 0u;
    return psx_audio_buffer_read(&machine->audio_output, interleaved_stereo,
                                 frame_capacity);
}
