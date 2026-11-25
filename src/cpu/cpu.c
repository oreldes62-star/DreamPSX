#include "dreampsx/cpu.h"
#include <string.h>

void psx_cpu_reset(PsxCpu *cpu)
{
    if (!cpu) return;
    memset(cpu, 0, sizeof(*cpu));
    cpu->pc = PSX_CPU_RESET_PC;
    cpu->next_pc = PSX_CPU_RESET_PC + 4u;
    cpu->initialized = true;
}

uint32_t psx_cpu_read_gpr(const PsxCpu *cpu, unsigned index)
{
    if (!cpu || !cpu->initialized || index >= 32u || index == 0u) return 0u;
    return cpu->gpr[index];
}

void psx_cpu_write_gpr(PsxCpu *cpu, unsigned index, uint32_t value)
{
    if (!cpu || !cpu->initialized || index == 0u || index >= 32u) return;
    cpu->gpr[index] = value;
}

bool psx_cpu_fetch_instruction(const PsxCpu *cpu, const PsxBus *bus,
                               uint32_t *instruction)
{
    if (!cpu || !cpu->initialized || !bus || !instruction) return false;
    return psx_bus_read32(bus, cpu->pc, instruction);
}
