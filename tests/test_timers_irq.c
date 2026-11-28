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

    /* Timer 0 selected for dot clock ignores system ticks. */
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
