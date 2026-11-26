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
    uint16_t *gpu_vram = (uint16_t *)calloc(PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT, sizeof(uint16_t));
    uint32_t word = 0u;
    uint16_t half = 0u;

    assert(ram && bios && gpu_vram);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE,
                            gpu_vram, PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
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

    /* GPU registers share the machine bus and update its VRAM state. */
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0x0200ff00u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 0u));
    assert(psx_bus_write32(&machine.bus, 0x1f801810u, 1u | (1u << 16u)));
    assert(psx_gpu_vram_read(&machine.gpu, 0u, 0u, &half));
    assert(half == 0x03e0u);
    assert(psx_bus_read32(&machine.bus, 0xbf801814u, &word));
    assert(word == 0x14802000u);

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

    /* GTE bus faults in a branch delay slot set Cause.BD and branch EPC. */
    machine.cop0_status = 0x40000000u; /* CU2 enabled */
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x220u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 0x224u,
                           (0x32u << 26) | (12u << 21))); /* LWC2 delay slot */
    machine.cpu.pc = 0x220u;
    machine.cpu.next_pc = 0x224u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x220u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x228u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 0x22cu,
                           (0x3au << 26) | (12u << 21))); /* SWC2 delay slot */
    machine.cpu.pc = 0x228u;
    machine.cpu.next_pc = 0x22cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x228u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    /* A not-taken branch still marks its following LWC2 as a delay slot. */
    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 8u, 7u);
    psx_cpu_write_gpr(&machine.cpu, 9u, 7u);
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x1f000000u);
    assert(psx_bus_write32(&machine.bus, 0x230u,
                           (0x05u << 26) | (8u << 21) | (9u << 16) | 1u)); /* BNE equal: not taken */
    assert(psx_bus_write32(&machine.bus, 0x234u,
                           (0x32u << 26) | (12u << 21))); /* LWC2 delay slot */
    machine.cpu.pc = 0x230u;
    machine.cpu.next_pc = 0x234u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(machine.cpu.delay_slot_pending);
    assert(machine.cpu.next_pc == 0x238u); /* sequential path, despite delay slot */
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x230u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x1f000000u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_BUS_DATA);

    /* Misaligned LWC2 in a branch delay slot reports AdEL with BD/EPC. */
    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x601u);
    assert(psx_bus_write32(&machine.bus, 0x240u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 0x244u,
                           (0x32u << 26) | (12u << 21))); /* LWC2 delay slot */
    machine.cpu.pc = 0x240u;
    machine.cpu.next_pc = 0x244u;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x240u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x601u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_ADDRESS_LOAD);

    /* Misaligned SWC2 in a branch delay slot reports AdES with BD/EPC. */
    machine.cop0_status = 0x40000000u;
    machine.cop0_cause = 0u;
    psx_cpu_write_gpr(&machine.cpu, 12u, 0x603u);
    assert(psx_bus_write32(&machine.bus, 0x248u, 0x10000001u)); /* BEQ zero,zero,+1 */
    assert(psx_bus_write32(&machine.bus, 0x24cu,
                           (0x3au << 26) | (12u << 21))); /* SWC2 delay slot */
    machine.cpu.pc = 0x248u;
    machine.cpu.next_pc = 0x24cu;
    machine.cpu.delay_slot_pending = false;
    machine.cpu.load_delay_pending = false;
    assert(psx_machine_step(&machine));
    assert(psx_machine_step(&machine));
    assert(machine.cop0_epc == 0x248u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x603u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_ADDRESS_STORE);

    free(gpu_vram);
    free(bios);
    free(ram);
    puts("DreamPSX machine-bus integration tests: OK");
    return 0;
}
