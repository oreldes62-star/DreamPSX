#ifndef DREAMPSX_CPU_H
#define DREAMPSX_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/bus.h"

#define PSX_CPU_RESET_PC 0xbfc00000u

typedef struct {
    uint32_t gpr[32];
    uint32_t hi;
    uint32_t lo;
    uint32_t pc;
    uint32_t next_pc;
    bool initialized;
} PsxCpu;

void psx_cpu_reset(PsxCpu *cpu);
uint32_t psx_cpu_read_gpr(const PsxCpu *cpu, unsigned index);
void psx_cpu_write_gpr(PsxCpu *cpu, unsigned index, uint32_t value);
bool psx_cpu_fetch_instruction(const PsxCpu *cpu, const PsxBus *bus,
                               uint32_t *instruction);

#endif
