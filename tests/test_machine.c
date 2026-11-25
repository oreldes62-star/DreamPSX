#include "dreampsx/machine.h"
#include "dreampsx/cop0_irq.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

int main(void)
{
    PsxMachine machine;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    assert(ram && bios);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE));
    assert(psx_memory_write32(&machine.memory, 0u, 0xdecafbad));
    uint32_t memory_probe = 0u;
    assert(psx_memory_read32(&machine.memory, 0x80000000u, &memory_probe));
    assert(memory_probe == 0xdecafbad);

    /* Reset establishes a clean IRQ/COP0 state. */
    assert(psx_irq_read_stat(&machine.irq) == 0u);
    assert(psx_irq_read_mask(&machine.irq) == 0u);
    assert(machine.cop0_status == 0u);
    assert(machine.cop0_cause == 0u);
    assert(!psx_machine_interrupt_eligible(&machine));

    /* Raise a source while masked: Cause.IP2 stays low. */
    psx_irq_set_line(&machine.irq, 2u, true);
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    /* Enable the source and CPU mask/global enable. */
    psx_irq_write_mask(&machine.irq, (uint16_t)(1u << 2));
    machine.cop0_status = PSX_COP0_STATUS_IEC | PSX_COP0_CAUSE_IP2;
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) != 0u);
    assert(psx_machine_interrupt_eligible(&machine));

    /* Disable CPU global enable: pending remains, delivery is inhibited. */
    machine.cop0_status &= ~PSX_COP0_STATUS_IEC;
    assert(!psx_machine_interrupt_eligible(&machine));
    assert((psx_irq_read_stat(&machine.irq) & (1u << 2)) != 0u);

    /* Acknowledge source; refresh deasserts IP2. */
    psx_irq_write_stat(&machine.irq, (uint16_t)~(1u << 2));
    psx_machine_refresh_interrupts(&machine);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_IP2) == 0u);

    free(bios);
    free(ram);
    puts("DreamPSX machine-state tests: OK");
    return 0;
}
