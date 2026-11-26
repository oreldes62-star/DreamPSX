#ifndef DREAMPSX_CPU_H
#define DREAMPSX_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/bus.h"

#define PSX_CPU_RESET_PC 0xbfc00000u

typedef enum {
    PSX_CPU_STEP_OK = 0,
    PSX_CPU_STEP_FETCH_BUS_ERROR,
    PSX_CPU_STEP_DATA_BUS_ERROR,
    PSX_CPU_STEP_ADDRESS_ERROR_LOAD,
    PSX_CPU_STEP_ADDRESS_ERROR_STORE,
    PSX_CPU_STEP_RESERVED_INSTRUCTION,
    PSX_CPU_STEP_SYSCALL,
    PSX_CPU_STEP_BREAKPOINT,
    PSX_CPU_STEP_OVERFLOW,
    PSX_CPU_STEP_COPROCESSOR_UNUSABLE,
    PSX_CPU_STEP_UNPREDICTABLE
} PsxCpuStepResult;

typedef struct {
    uint32_t gpr[32];
    uint32_t hi;
    uint32_t lo;
    uint32_t pc;
    uint32_t next_pc;
    uint32_t delay_branch_pc;
    uint32_t fault_address;
    uint32_t load_delay_value;
    uint8_t load_delay_register;
    bool load_delay_pending;
    bool load_delay_mergeable;
    bool delay_slot_pending;
    bool initialized;
} PsxCpu;

void psx_cpu_reset(PsxCpu *cpu);
uint32_t psx_cpu_read_gpr(const PsxCpu *cpu, unsigned index);
void psx_cpu_write_gpr(PsxCpu *cpu, unsigned index, uint32_t value);
bool psx_cpu_fetch_instruction(const PsxCpu *cpu, const PsxBus *bus,
                               uint32_t *instruction);
/* Detailed result lets the machine translate CPU faults into COP0 exceptions. */
PsxCpuStepResult psx_cpu_step_result_with_cop0(
    PsxCpu *cpu, PsxBus *bus, uint32_t *status, uint32_t *cause,
    uint32_t *epc, uint32_t *badvaddr);
/* Compatibility wrappers: true only when an instruction executed. */
bool psx_cpu_step(PsxCpu *cpu, PsxBus *bus);
bool psx_cpu_step_with_cop0(PsxCpu *cpu, PsxBus *bus,
                            uint32_t *status, uint32_t *cause,
                            uint32_t *epc, uint32_t *badvaddr);

#endif
