#include "dreampsx/machine.h"
#include "dreampsx/cop0_irq.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    PsxMachine machine;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    uint16_t *vram = (uint16_t *)calloc(
        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT, sizeof(uint16_t));
    assert(ram && bios && vram);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios,
                            PSX_BIOS_SIZE, vram,
                            PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    /* Enable timer IRQ sources in I_MASK before checking the CPU IRQ output. */
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR,
                           (uint16_t)((1u << 4u) | (1u << 5u) | (1u << 6u))));

    /* Timer 1 target event is routed to PS1 IRQ source 5 (I_STAT bit 5). */
    assert(psx_bus_write16(&machine.bus,
        PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 8u, 2u));
    assert(psx_bus_write16(&machine.bus,
        PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u,
        PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_machine_timer_advance_clock(&machine, 1u,
        PSX_TIMER_CLOCK_SYSTEM, 2u));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 5u)) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* Acknowledge the latched request; one-shot mode must not retrigger. */
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 5u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 5u)) == 0u);
    assert(psx_machine_timer_advance_clock(&machine, 1u,
        PSX_TIMER_CLOCK_SYSTEM, 2u));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 5u)) == 0u);

    /* A counter wrap with TARGET=0 reaches target and overflow on the
     * same tick. Both mode flags are read-to-clear, but I_STAT is independent. */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR,
                           (uint16_t)(1u << 4u)));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR, 0xffffu));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 0u));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_IRQ_TARGET |
                   PSX_TIMER_MODE_IRQ_OVERFLOW)));
    /* MODE writes reset the counter, so position it at FFFF afterwards. */
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR, 0xffffu));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(machine.timers.timer[0].counter == 0u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    {
        uint16_t mode = 0u;
        assert(psx_bus_read16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u, &mode));
        assert((mode & (PSX_TIMER_MODE_REACHED_TARGET |
                        PSX_TIMER_MODE_REACHED_OVERFLOW)) ==
               (PSX_TIMER_MODE_REACHED_TARGET |
                PSX_TIMER_MODE_REACHED_OVERFLOW));
        assert(psx_bus_read16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u, &mode));
        assert((mode & (PSX_TIMER_MODE_REACHED_TARGET |
                        PSX_TIMER_MODE_REACHED_OVERFLOW)) == 0u);
    }
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 4u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) == 0u);

    /* IRQ toggle mode alternates the timer output on each repeated target.
     * Only low-to-high transitions create a fresh I_STAT request. */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR,
                           (uint16_t)(1u << 4u)));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_RESET_AT_TARGET |
                   PSX_TIMER_MODE_IRQ_TARGET | PSX_TIMER_MODE_IRQ_REPEAT |
                   PSX_TIMER_MODE_IRQ_TOGGLE)));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(machine.timers.timer[0].irq_toggle_state);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 4u));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(!machine.timers.timer[0].irq_toggle_state);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) == 0u);
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(machine.timers.timer[0].irq_toggle_state);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 4u));
    psx_machine_refresh_interrupts(&machine);
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert(!machine.timers.timer[0].irq_toggle_state);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) == 0u);

    /* MODE writes reset each timer's toggle output line. The I_STAT edge
     * latch remains independent. Exercise all three IRQ source mappings and
     * both bus write widths (the timer register itself is 16-bit). */
    for (unsigned timer = 0u; timer < PSX_TIMER_COUNT; ++timer) {
        const uint32_t mode_address = PSX_TIMER_BASE_ADDR +
            timer * PSX_TIMER_STRIDE + 4u;
        const uint32_t target_address = PSX_TIMER_BASE_ADDR +
            timer * PSX_TIMER_STRIDE + 8u;
        const uint16_t irq_bit = (uint16_t)(1u << (4u + timer));
        const uint16_t toggle_mode = (uint16_t)(
            PSX_TIMER_MODE_RESET_AT_TARGET |
            PSX_TIMER_MODE_IRQ_TARGET | PSX_TIMER_MODE_IRQ_REPEAT |
            PSX_TIMER_MODE_IRQ_TOGGLE);

        psx_machine_reset(&machine);
        assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR, irq_bit));
        assert(psx_bus_write16(&machine.bus, target_address, 1u));
        assert(psx_bus_write16(&machine.bus, mode_address, toggle_mode));
        assert(psx_machine_timer_advance_clock(&machine, timer,
            PSX_TIMER_CLOCK_SYSTEM, 1u));
        assert(machine.timers.timer[timer].irq_toggle_state);
        assert((machine.irq.source_level & irq_bit) != 0u);
        assert((psx_irq_read_stat(&machine.irq) & irq_bit) != 0u);

        if (timer == 1u)
            assert(psx_bus_write32(&machine.bus, mode_address, toggle_mode));
        else
            assert(psx_bus_write16(&machine.bus, mode_address, toggle_mode));

        assert(!machine.timers.timer[timer].irq_toggle_state);
        assert((machine.irq.source_level & irq_bit) == 0u);
        assert((psx_irq_read_stat(&machine.irq) & irq_bit) != 0u);

        psx_irq_write_stat(&machine.irq, (uint16_t)~irq_bit);
        psx_machine_refresh_interrupts(&machine);
        assert((psx_irq_read_stat(&machine.irq) & irq_bit) == 0u);
        assert(psx_machine_timer_advance_clock(&machine, timer,
            PSX_TIMER_CLOCK_SYSTEM, 1u));
        assert(machine.timers.timer[timer].irq_toggle_state);
        assert((psx_irq_read_stat(&machine.irq) & irq_bit) != 0u);
    }

    /* Timer2's divide-by-eight source retains sub-tick phase across calls. */
    assert(psx_bus_write16(&machine.bus,
        PSX_TIMER_BASE_ADDR + 2u * PSX_TIMER_STRIDE + 8u, 1u));
    assert(psx_bus_write16(&machine.bus,
        PSX_TIMER_BASE_ADDR + 2u * PSX_TIMER_STRIDE + 4u,
        (uint16_t)((1u << PSX_TIMER_MODE_CLOCK_SHIFT) |
                   PSX_TIMER_MODE_IRQ_TARGET)));
    assert(psx_machine_advance_system_cycles(&machine, 3u));
    assert(machine.timers.timer[2].counter == 0u);
    assert(machine.timer2_div8_phase == 3u);
    assert(psx_machine_advance_system_cycles(&machine, 4u));
    assert(machine.timers.timer[2].counter == 0u);
    assert(machine.timer2_div8_phase == 7u);
    assert(psx_machine_advance_system_cycles(&machine, 1u));
    assert(machine.timers.timer[2].counter == 1u);
    assert(machine.timer2_div8_phase == 0u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 6u)) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* Timer 0 selected for dot clock ignores system ticks and routes its
     * target event to source 4 (I_STAT bit 4), independently of Timer 1/2. */
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)((1u << PSX_TIMER_MODE_CLOCK_SHIFT) |
                   PSX_TIMER_MODE_IRQ_TARGET)));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 3u));
    assert(machine.timers.timer[0].counter == 0u);
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_DOT, 1u));
    assert(machine.timers.timer[0].counter == 1u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* Repeating target IRQs must create a fresh controller edge after
     * software acknowledges the previous I_STAT request. */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR,
                           (uint16_t)(1u << 4u)));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_RESET_AT_TARGET |
                   PSX_TIMER_MODE_IRQ_TARGET | PSX_TIMER_MODE_IRQ_REPEAT)));
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 4u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) == 0u);
    assert(psx_machine_timer_advance_clock(&machine, 0u,
        PSX_TIMER_CLOCK_SYSTEM, 1u));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);

    /* A real machine instruction now advances the shared system clock.
     * The timer IRQ is latched after retirement and sampled at the next
     * instruction boundary.
     */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR,
                           (uint16_t)(1u << 4u)));
    assert(psx_bus_write32(&machine.bus, 0u, 0x24080001u)); /* ADDIU t0,zero,1 */
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u,
                           PSX_TIMER_MODE_IRQ_TARGET));
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    assert(psx_machine_step(&machine));
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 1u);
    assert(psx_cpu_last_step_cycles(&machine.cpu) == 1u);
    assert(machine.timers.timer[0].counter == 1u);
    assert((psx_irq_read_stat(&machine.irq) & (1u << 4u)) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* The timer request must reach the R3000A exception boundary. The
     * first instruction raises Timer 0; the following step takes the
     * external interrupt before executing the next instruction. */
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    machine.cpu.pc = 4u;
    machine.cpu.next_pc = 8u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 4u);
    assert((machine.cop0_status & PSX_COP0_STATUS_IEC) == 0u);

    /* Three timer sources may be pending together. Taking the CPU
     * interrupt must preserve all I_STAT latches; acknowledging one source
     * must leave the other two asserted on the shared COP0 IP2 line. */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR,
        (uint16_t)((1u << 4u) | (1u << 5u) | (1u << 6u))));
    for (unsigned timer = 0u; timer < 3u; ++timer) {
        const uint32_t base = PSX_TIMER_BASE_ADDR + timer * PSX_TIMER_STRIDE;
        assert(psx_bus_write16(&machine.bus, base + 8u, 1u));
        assert(psx_bus_write16(&machine.bus, base + 4u,
                               PSX_TIMER_MODE_IRQ_TARGET));
        assert(psx_machine_timer_advance_clock(&machine, timer,
            PSX_TIMER_CLOCK_SYSTEM, 1u));
    }
    assert((psx_irq_read_stat(&machine.irq) & 0x70u) == 0x70u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_bus_write32(&machine.bus, 0u, 0x24080001u));
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert((psx_irq_read_stat(&machine.irq) & 0x70u) == 0x70u);
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 5u));
    psx_machine_refresh_interrupts(&machine);
    assert((psx_irq_read_stat(&machine.irq) & 0x70u) == 0x50u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);

    /* An IRQ arriving after a taken branch must not split its delay slot.
     * The delay-slot instruction retires first; the interrupt is accepted
     * at the following boundary with EPC pointing at the branch target. */
    psx_machine_reset(&machine);
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR,
                           (uint16_t)(1u << 4u)));
    assert(psx_bus_write32(&machine.bus, 0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x24090007u)); /* ADDIU t1,zero,7 */
    assert(psx_bus_write32(&machine.bus, 8u, 0x240a0009u)); /* ADDIU t2,zero,9 */
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 8u, 1u));
    assert(psx_bus_write16(&machine.bus, PSX_TIMER_BASE_ADDR + 4u,
                           PSX_TIMER_MODE_IRQ_TARGET));
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 10u);
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    assert(psx_machine_step(&machine)); /* Branch raises Timer 0 IRQ */
    assert(machine.cpu.delay_slot_pending);
    assert(machine.cpu.pc == 4u && machine.cpu.next_pc == 8u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_step(&machine)); /* IRQ deferred: execute delay slot */
    assert(!machine.cpu.delay_slot_pending);
    assert(psx_cpu_read_gpr(&machine.cpu, 9u) == 7u);
    assert(machine.cpu.pc == 8u && machine.cpu.next_pc == 12u);
    assert(psx_machine_step(&machine)); /* Now accept interrupt */
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 8u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) == 0u);
    assert(psx_cpu_read_gpr(&machine.cpu, 10u) == 0u);

    /* A synchronous exception raised by the delay-slot instruction
     * must report the branch address in EPC and set Cause.BD. */
    psx_machine_reset(&machine);
    assert(psx_bus_write32(&machine.bus, 0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x0000000cu)); /* SYSCALL */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert((machine.cop0_cause & 0x7cu) ==
           ((uint32_t)PSX_EXCEPTION_SYSCALL << 2u));

    /* Address errors in a delay slot must additionally preserve the
     * faulting virtual address in BadVAddr while EPC identifies the branch. */
    psx_machine_reset(&machine);
    assert(psx_bus_write32(&machine.bus, 0u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 4u, 0x8c080001u)); /* LW t0,1(zero) */
    machine.cpu.pc = 0u;
    machine.cpu.next_pc = 4u;
    assert(psx_machine_step(&machine));
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cop0_epc == 0u);
    assert(machine.cop0_badvaddr == 1u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) ==
           ((uint32_t)PSX_EXCEPTION_ADDRESS_LOAD <<
            PSX_COP0_CAUSE_EXCCODE_SHIFT));

    /* Clearing I_STAT removes IP2; a fresh edge is required to retrigger. */
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 4u));
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    free(vram);
    free(bios);
    free(ram);
    puts("DreamPSX timer IRQ integration tests: OK");
    return 0;
}
