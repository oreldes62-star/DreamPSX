#include "dreampsx/cop0.h"
#include "dreampsx/cop0_irq.h"
#include "dreampsx/machine.h"
#include <assert.h>
#include <stdint.h>
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

    /* Software interrupt IP0 is independent of the external IRQ controller.
     * Refreshing device IRQs must preserve it and the unrelated Cause bits. */
    machine.cpu.pc = 0x80001000u;
    machine.cpu.next_pc = 0x80001004u;
    machine.cop0_status = PSX_COP0_STATUS_IEC | (1u << 8u);
    machine.cop0_cause = 0x80000000u | (1u << 8u);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & (1u << 8u)) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x80001000u);
    assert(machine.cpu.pc == 0x80000080u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE_MASK) == 0u);
    assert((machine.cop0_cause & 0x80000100u) == 0x80000100u);

    /* A pending IP1 is ignored while IM1 is clear, then accepted as soon
     * as its Status mask is enabled. */
    machine.cpu.pc = 0x80002000u;
    machine.cpu.next_pc = 0x80002004u;
    machine.cop0_status = PSX_COP0_STATUS_IEC;
    machine.cop0_cause = (1u << 9u);
    psx_machine_refresh_interrupts(&machine);
    assert(!psx_machine_interrupt_eligible(&machine));
    assert(psx_machine_step(&machine));
    assert(machine.cpu.pc == 0x80002004u);
    machine.cop0_status |= (1u << 9u);
    assert(psx_machine_interrupt_eligible(&machine));
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x80002004u);
    assert(machine.cpu.pc == 0x80000080u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE_MASK) == 0u);
    assert((machine.cop0_cause & (1u << 9u)) != 0u);

    free(vram);
    free(bios);
    free(ram);
    return 0;
}
