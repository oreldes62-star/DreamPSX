#include "dreampsx/machine.h"
#include "dreampsx/irq.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    PsxMachine machine;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    uint32_t word = 0u;
    uint16_t half = 0u;

    assert(ram && bios);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE));
    assert(machine.bus.initialized);

    /* CPU-facing bus reaches RAM and its KSEG aliases. */
    assert(psx_bus_write32(&machine.bus, 0x00000100u, 0x1234abcdu));
    assert(psx_bus_read32(&machine.bus, 0x80000100u, &word));
    assert(word == 0x1234abcdu);
    assert(psx_bus_read32(&machine.bus, 0xa0000100u, &word));
    assert(word == 0x1234abcdu);

    /* The same bus reaches IRQ controller through physical and KSEG1 MMIO. */
    assert(psx_bus_write16(&machine.bus, PSX_IRQ_MASK_ADDR, (uint16_t)(1u << 2)));
    assert(psx_bus_read16(&machine.bus, 0xbf801074u, &half));
    assert(half == (uint16_t)(1u << 2));

    psx_irq_set_line(&machine.irq, 2u, true);
    assert(psx_bus_read16(&machine.bus, 0x1f801070u, &half));
    assert((half & (1u << 2)) != 0u);

    /* I_STAT is write-zero-to-clear through the CPU-facing bus. */
    assert(psx_bus_write16(&machine.bus, 0x1f801070u,
                           (uint16_t)~(1u << 2)));
    assert(psx_bus_read16(&machine.bus, PSX_IRQ_STAT_ADDR, &half));
    assert((half & (1u << 2)) == 0u);

    /* BIOS is readable; unmapped hardware and misaligned words are rejected. */
    assert(psx_bus_read32(&machine.bus, PSX_BIOS_BASE, &word));
    assert(!psx_bus_read32(&machine.bus, 0x1f801080u, &word));
    assert(!psx_bus_read32(&machine.bus, 0x00000101u, &word));

    /* LWC2/SWC2 unmapped accesses become COP0 data-bus exceptions. */
    machine.cop0_status = 0x40000000u; /* CU2 enabled */
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x200u,
                           (0x32u << 26) | (12u << 21))); /* LWC2 D0,0(t4) */
    machine.cpu.pc = 0x200u;
    machine.cpu.next_pc = 0x204u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x200u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x204u,
                           (0x3au << 26) | (12u << 21))); /* SWC2 D0,0(t4) */
    machine.cpu.pc = 0x204u;
    machine.cpu.next_pc = 0x208u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x204u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    free(bios);
    free(ram);
    puts("DreamPSX machine-bus integration tests: OK");
    return 0;
}
