#include "dreampsx/machine.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

int main(void)
{
    PsxMachine machine;
    static uint16_t gpu_vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint8_t *ram = (uint8_t *)calloc(1u, PSX_RAM_SIZE);
    uint8_t *bios = (uint8_t *)calloc(1u, PSX_BIOS_SIZE);
    assert(ram && bios);
    assert(psx_machine_init(&machine, ram, PSX_RAM_SIZE, bios, PSX_BIOS_SIZE,
                            gpu_vram, PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));

    machine.cpu.pc = 0x80001004u;
    machine.cpu.next_pc = 0x80001008u;
    machine.cop0_status = 0x0000000bu; /* current IE/KU and previous mode bits */
    machine.cop0_cause = 0x00000700u;  /* preserve software IP0/IP1 and hardware IP2 */
    psx_machine_raise_exception(&machine, PSX_EXCEPTION_OVERFLOW,
                                0u, false, 0u);
    assert(machine.cop0_epc == 0x80001004u);
    assert(((machine.cop0_cause & PSX_COP0_CAUSE_EXCCODE) >>
            PSX_COP0_CAUSE_EXCCODE_SHIFT) == PSX_EXCEPTION_OVERFLOW);
    assert((machine.cop0_cause & 0x00000700u) == 0x00000700u);
    assert(machine.cpu.pc == 0x80000080u);
    assert(machine.cpu.next_pc == 0x80000084u);

    /* Delay-slot exception records the branch address and sets Cause.BD. */
    machine.cpu.pc = 0x80002004u;
    machine.cop0_status = PSX_COP0_STATUS_BEV | 0x0bu;
    psx_machine_raise_exception(&machine, PSX_EXCEPTION_ADDRESS_LOAD,
                                0x12345678u, true, 0x80002000u);
    assert(machine.cop0_epc == 0x80002000u);
    assert((machine.cop0_cause & PSX_COP0_CAUSE_BD) != 0u);
    assert(machine.cop0_badvaddr == 0x12345678u);
    assert(machine.cpu.pc == 0xbfc00180u);
    assert(machine.cpu.next_pc == 0xbfc00184u);
    assert((machine.cop0_cause & 0x00000700u) == 0x00000700u);

    free(bios);
    free(ram);
    return 0;
}
