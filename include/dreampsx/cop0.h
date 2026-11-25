#ifndef DREAMPSX_COP0_H
#define DREAMPSX_COP0_H

#include <stdbool.h>
#include <stdint.h>

/* MIPS-I / PlayStation COP0 register fields. */
#define PSX_COP0_STATUS_KUC      (1u << 1)
#define PSX_COP0_CAUSE_SOFTWARE_IP 0x00000300u
#define PSX_COP0_PRID_R3000A     0x00000002u
#define PSX_COP0_STATUS_BEV       (1u << 22)
#define PSX_COP0_STATUS_EXCEPTION_STACK 0x0000003fu
#define PSX_COP0_CAUSE_BD         (1u << 31)
#define PSX_COP0_CAUSE_EXCCODE    0x0000007cu
#define PSX_COP0_CAUSE_EXCCODE_SHIFT 2u

typedef enum {
    PSX_EXCEPTION_INTERRUPT = 0,
    PSX_EXCEPTION_TLB_MODIFICATION = 1,
    PSX_EXCEPTION_TLB_LOAD = 2,
    PSX_EXCEPTION_TLB_STORE = 3,
    PSX_EXCEPTION_ADDRESS_LOAD = 4,
    PSX_EXCEPTION_ADDRESS_STORE = 5,
    PSX_EXCEPTION_BUS_INSTRUCTION = 6,
    PSX_EXCEPTION_BUS_DATA = 7,
    PSX_EXCEPTION_SYSCALL = 8,
    PSX_EXCEPTION_BREAKPOINT = 9,
    PSX_EXCEPTION_RESERVED_INSTRUCTION = 10,
    PSX_EXCEPTION_COPROCESSOR_UNUSABLE = 11,
    PSX_EXCEPTION_OVERFLOW = 12
} PsxExceptionCode;

/* Apply the R3000A status-stack and Cause/EPC rules.
 * Returns the exception vector selected by Status.BEV.
 */
uint32_t psx_cop0_exception_entry(uint32_t *status, uint32_t *cause,
                                 uint32_t *epc, PsxExceptionCode code,
                                 uint32_t current_pc, bool in_delay_slot,
                                 uint32_t branch_pc);

#endif
