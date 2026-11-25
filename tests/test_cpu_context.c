#include "dreampsx/cpu.h"
#include "dreampsx/machine.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    PsxMachine machine;
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    uint32_t instruction = 0u;
    assert(ram && bios);
    bios[0] = 0x2du; bios[1] = 0x08u; bios[2] = 0x00u; bios[3] = 0x3cu;
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE));
    assert(machine.cpu.initialized);
    assert(machine.cpu.pc == PSX_CPU_RESET_PC);
    assert(machine.cpu.next_pc == PSX_CPU_RESET_PC + 4u);
    assert(psx_cpu_fetch_instruction(&machine.cpu, &machine.bus, &instruction));
    assert(instruction == 0x3c00082du);
    psx_cpu_write_gpr(&machine.cpu, 0u, 0xffffffffu);
    assert(psx_cpu_read_gpr(&machine.cpu, 0u) == 0u);
    psx_cpu_write_gpr(&machine.cpu, 8u, 0xabcdef01u);
    assert(psx_cpu_read_gpr(&machine.cpu, 8u) == 0xabcdef01u);
    assert(psx_cpu_read_gpr(&machine.cpu, 32u) == 0u);
    free(bios); free(ram);
    puts("DreamPSX CPU context tests: OK");
    return 0;
}
