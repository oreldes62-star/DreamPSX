#include "dreampsx/cpu.h"
#include <string.h>

static uint32_t sign_extend_16(uint32_t value)
{
    value &= 0xffffu;
    return (value & 0x8000u) ? (value | 0xffff0000u) : value;
}

static int64_t signed32(uint32_t value)
{
    return (value & 0x80000000u) ? (int64_t)value - 0x100000000LL
                                  : (int64_t)value;
}

static bool signed_less(uint32_t a, uint32_t b)
{
    bool sa = (a >> 31) != 0u;
    bool sb = (b >> 31) != 0u;
    return sa != sb ? sa : a < b;
}

static uint32_t arithmetic_shift_right(uint32_t value, unsigned amount)
{
    uint32_t shifted;
    if (amount == 0u) return value;
    shifted = value >> amount;
    if ((value & 0x80000000u) != 0u)
        shifted |= 0xffffffffu << (32u - amount);
    return shifted;
}

void psx_cpu_reset(PsxCpu *cpu)
{
    if (!cpu) return;
    memset(cpu, 0, sizeof(*cpu));
    cpu->pc = PSX_CPU_RESET_PC;
    cpu->next_pc = PSX_CPU_RESET_PC + 4u;
    cpu->initialized = true;
}

uint32_t psx_cpu_last_step_cycles(const PsxCpu *cpu)
{
    if (!cpu || !cpu->initialized) return 0u;
    return cpu->last_step_cycles;
}

static void cpu_commit_hilo(PsxCpu *cpu)
{
    if (!cpu || !cpu->hilo_pending) return;
    cpu->hi = cpu->hilo_pending_hi;
    cpu->lo = cpu->hilo_pending_lo;
    cpu->hilo_pending = false;
    cpu->hilo_busy_cycles = 0u;
}

static uint8_t cpu_multiply_cycles(uint32_t value, bool is_signed)
{
    if (is_signed) {
        if (value <= 0x000007ffu || value >= 0xfffff800u) return 6u;
        if ((value >= 0x00000800u && value <= 0x000fffffu) ||
            (value >= 0xfff00000u && value <= 0xfffff7ffu)) return 9u;
        return 13u;
    }
    if (value <= 0x000007ffu) return 6u;
    if (value <= 0x000fffffu) return 9u;
    return 13u;
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

PsxCpuStepResult psx_cpu_step_result_with_cop0_gte(PsxCpu *cpu, PsxBus *bus,
                            uint32_t *cop0_status, uint32_t *cop0_cause,
                            uint32_t *cop0_epc, uint32_t *cop0_badvaddr,
                            PsxGte *gte)
{
    uint32_t instruction, old_pc, sequential_pc, sequential_next;
    unsigned op, rs, rt, rd, shamt, funct;
    uint32_t a, b, result = 0u, address;
    uint16_t imm;
    bool write_result = false, branch = false;
    bool control_transfer = false;
    bool was_delay_slot;
    bool is_load = false;
    bool is_merge_load = false;
    uint32_t previous_load_value = 0u;
    unsigned previous_load_register = 0u;
    bool previous_load_pending = false;
    bool previous_load_mergeable = false;
    uint32_t branch_target = 0u;
    uint8_t byte_value;
    uint16_t half_value;
    bool hilo_interlock;
    bool gte_interlock;

    if (!cpu || !cpu->initialized || !bus)
        return PSX_CPU_STEP_FETCH_BUS_ERROR;
    /* Conservative baseline: one system cycle per attempted instruction.
     * Bus wait states and instruction-specific latency are future work. */
    cpu->last_step_cycles = 1u;
    if ((cpu->pc & 3u) != 0u) {
        cpu->fault_address = cpu->pc;
        return PSX_CPU_STEP_ADDRESS_ERROR_LOAD;
    }
    if (!psx_cpu_fetch_instruction(cpu, bus, &instruction)) {
        cpu->fault_address = cpu->pc;
        return PSX_CPU_STEP_FETCH_BUS_ERROR;
    }

    old_pc = cpu->pc;
    was_delay_slot = cpu->delay_slot_pending;
    previous_load_pending = cpu->load_delay_pending;
    previous_load_mergeable = cpu->load_delay_mergeable;
    previous_load_register = cpu->load_delay_register;
    previous_load_value = cpu->load_delay_value;
    /* Branches in a delay slot are architecturally unpredictable on R3000A. */
    if (was_delay_slot &&
        (((instruction >> 26) == 0x01u) ||
         ((instruction >> 26) >= 0x02u && (instruction >> 26) <= 0x07u) ||
         ((instruction >> 26) == 0u &&
          (((instruction & 63u) == 0x08u) || ((instruction & 63u) == 0x09u)))))
        return PSX_CPU_STEP_UNPREDICTABLE;
    sequential_pc = cpu->next_pc;
    sequential_next = cpu->next_pc + 4u;
    op = instruction >> 26;
    rs = (instruction >> 21) & 31u;
    rt = (instruction >> 16) & 31u;
    rd = (instruction >> 11) & 31u;
    shamt = (instruction >> 6) & 31u;
    funct = instruction & 63u;
    imm = (uint16_t)instruction;
    gte_interlock = (op == 0x12u || op == 0x32u || op == 0x3au) &&
                    gte && cop0_status &&
                    ((*cop0_status & 0x40000000u) != 0u) &&
                    psx_gte_busy_cycles(gte) != 0u;
    if (gte && psx_gte_busy_cycles(gte) != 0u) {
        if (gte_interlock) {
            uint32_t wait = psx_gte_busy_cycles(gte);
            cpu->last_step_cycles += wait;
            psx_gte_advance_cycles(gte, wait);
        } else {
            psx_gte_advance_cycles(gte, 1u);
        }
    }
    hilo_interlock = op == 0u &&
        (funct == 0x10u || funct == 0x11u ||
         funct == 0x12u || funct == 0x13u ||
         funct == 0x18u || funct == 0x19u ||
         funct == 0x1au || funct == 0x1bu);
    if (cpu->hilo_busy_cycles != 0u) {
        if (hilo_interlock) {
            /* HI/LO consumers and a new HI/LO operation wait for the unit. */
            cpu->last_step_cycles += cpu->hilo_busy_cycles;
            cpu_commit_hilo(cpu);
        } else {
            --cpu->hilo_busy_cycles;
            if (cpu->hilo_busy_cycles == 0u)
                cpu_commit_hilo(cpu);
        }
    }
    a = psx_cpu_read_gpr(cpu, rs);
    b = psx_cpu_read_gpr(cpu, rt);
    /* MIPS I special case: adjacent LWL/LWR to the same rt merge through
     * the pending load value instead of the stale architectural GPR value.
     */
    if ((op == 0x22u || op == 0x26u) && previous_load_pending &&
        previous_load_mergeable && previous_load_register == rt)
        b = previous_load_value;

    switch (op) {
    case 0x00u:
        /* Validate fixed fields for the SPECIAL encodings implemented below. */
        switch (funct) {
        case 0x00u: case 0x02u: case 0x03u:
            if (rs != 0u) return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        case 0x04u: case 0x06u: case 0x07u:
            if (shamt != 0u) return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        case 0x08u:
            if (rt != 0u || rd != 0u || shamt != 0u)
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        case 0x09u:
            if (rt != 0u || shamt != 0u)
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        case 0x10u: case 0x12u:
            if (rs != 0u || rt != 0u || shamt != 0u)
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        case 0x11u: case 0x13u:
            if (rt != 0u || rd != 0u || shamt != 0u)
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        case 0x18u: case 0x19u: case 0x1au: case 0x1bu:
            if (rd != 0u || shamt != 0u)
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        case 0x21u: case 0x23u: case 0x24u: case 0x25u:
        case 0x26u: case 0x27u: case 0x2au: case 0x2bu:
            if (shamt != 0u) return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        case 0x0cu: case 0x0du: /* SYSCALL/BREAK code field is variable. */
        case 0x30u: case 0x31u: case 0x32u:
        case 0x33u: case 0x34u: case 0x36u: /* Trap code field is variable. */
            break;
        default:
            break;
        }
        switch (funct) {
        case 0x00u: result = b << shamt; write_result = true; break;
        case 0x02u: result = b >> shamt; write_result = true; break;
        case 0x03u: result = arithmetic_shift_right(b, shamt); write_result = true; break;
        case 0x04u: result = b << (a & 31u); write_result = true; break; /* SLLV */
        case 0x06u: result = b >> (a & 31u); write_result = true; break; /* SRLV */
        case 0x07u: result = arithmetic_shift_right(b, a & 31u); write_result = true; break; /* SRAV */
        case 0x08u: control_transfer = true; branch = true; branch_target = a; break; /* JR */
        case 0x0cu: return PSX_CPU_STEP_SYSCALL;
        case 0x0du: return PSX_CPU_STEP_BREAKPOINT;
        case 0x30u: if (signed_less(a, b) == 0u) return PSX_CPU_STEP_TRAP; break;
        case 0x31u: if (a >= b) return PSX_CPU_STEP_TRAP; break;
        case 0x32u: if (signed_less(a, b) != 0u) return PSX_CPU_STEP_TRAP; break;
        case 0x33u: if (a < b) return PSX_CPU_STEP_TRAP; break;
        case 0x34u: if (a == b) return PSX_CPU_STEP_TRAP; break;
        case 0x36u: if (a != b) return PSX_CPU_STEP_TRAP; break;
        case 0x09u: /* JALR */
            result = old_pc + 8u;
            write_result = true;
            control_transfer = true; branch = true; branch_target = a;
            break;
        case 0x10u: result = cpu->hi; write_result = true; break; /* MFHI */
        case 0x11u: cpu->hi = a; break; /* MTHI */
        case 0x12u: result = cpu->lo; write_result = true; break; /* MFLO */
        case 0x13u: cpu->lo = a; break; /* MTLO */
        case 0x18u: { /* MULT: asynchronous result with operand-dependent latency. */
            int64_t product = signed32(a) * signed32(b);
            uint64_t bits = (uint64_t)product;
            cpu->hilo_pending_lo = (uint32_t)bits;
            cpu->hilo_pending_hi = (uint32_t)(bits >> 32);
            cpu->hilo_pending = true;
            cpu->hilo_busy_cycles = (uint8_t)(cpu_multiply_cycles(a, true) - 1u);
            break;
        }
        case 0x19u: { /* MULTU */
            uint64_t product = (uint64_t)a * (uint64_t)b;
            cpu->hilo_pending_lo = (uint32_t)product;
            cpu->hilo_pending_hi = (uint32_t)(product >> 32);
            cpu->hilo_pending = true;
            cpu->hilo_busy_cycles = (uint8_t)(cpu_multiply_cycles(a, false) - 1u);
            break;
        }
        case 0x1au: /* DIV: result becomes visible after 36 cycles. */
            if (b == 0u) {
                cpu->hilo_pending_lo = (a & 0x80000000u) ? 1u : 0xffffffffu;
                cpu->hilo_pending_hi = a;
            } else if (a == 0x80000000u && b == 0xffffffffu) {
                cpu->hilo_pending_lo = 0x80000000u;
                cpu->hilo_pending_hi = 0u;
            } else {
                int64_t sa = signed32(a), sb = signed32(b);
                cpu->hilo_pending_lo = (uint32_t)(sa / sb);
                cpu->hilo_pending_hi = (uint32_t)(sa % sb);
            }
            cpu->hilo_pending = true;
            cpu->hilo_busy_cycles = 35u;
            break;
        case 0x1bu: /* DIVU */
            if (b == 0u) {
                cpu->hilo_pending_lo = 0xffffffffu;
                cpu->hilo_pending_hi = a;
            } else {
                cpu->hilo_pending_lo = a / b;
                cpu->hilo_pending_hi = a % b;
            }
            cpu->hilo_pending = true;
            cpu->hilo_busy_cycles = 35u;
            break;
        case 0x21u: result = a + b; write_result = true; break;
        case 0x23u: result = a - b; write_result = true; break;
        case 0x24u: result = a & b; write_result = true; break;
        case 0x25u: result = a | b; write_result = true; break;
        case 0x26u: result = a ^ b; write_result = true; break;
        case 0x27u: result = ~(a | b); write_result = true; break;
        case 0x2au: result = signed_less(a, b); write_result = true; break;
        case 0x2bu: result = a < b; write_result = true; break;
        default: return PSX_CPU_STEP_RESERVED_INSTRUCTION;
        }
        break;
    case 0x12u: { /* COP2 register transfers and GTE commands. */
        unsigned cop_rs = rs;
        uint32_t cop_value;
        if (!cop0_status || ((*cop0_status & 0x40000000u) == 0u) || !gte)
            return PSX_CPU_STEP_COPROCESSOR_UNUSABLE;
        if ((cop_rs & 0x10u) != 0u) {
            if (!psx_gte_issue(gte, instruction))
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        }
        if ((instruction & 0x7ffu) != 0u)
            return PSX_CPU_STEP_RESERVED_INSTRUCTION;
        switch (cop_rs) {
        case 0u: /* MFC2: delayed move from GTE data register. */
            if (!psx_gte_read_data(gte, (uint8_t)rd, &cop_value))
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            result = cop_value;
            write_result = true;
            is_load = true;
            break;
        case 2u: /* CFC2: delayed move from GTE control register. */
            if (!psx_gte_read_control(gte, (uint8_t)rd, &cop_value))
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            result = cop_value;
            write_result = true;
            is_load = true;
            break;
        case 4u: /* MTC2 */
            if (!psx_gte_write_data(gte, (uint8_t)rd, b))
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        case 6u: /* CTC2 */
            if (!psx_gte_write_control(gte, (uint8_t)rd, b))
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            break;
        default:
            /* Reserved COP2 transfer encoding. */
            return PSX_CPU_STEP_RESERVED_INSTRUCTION;
        }
        break;
    }
    case 0x10u: { /* COP0: MFC0, MTC0 and RFE */
        unsigned cop_rs = rs;
        uint32_t cp_value;
        if (!cop0_status || !cop0_cause || !cop0_epc || !cop0_badvaddr ||
            ((*cop0_status & 0x00000002u) != 0u))
            return PSX_CPU_STEP_COPROCESSOR_UNUSABLE;
        if (cop_rs == 0u) { /* MFC0 */
            if ((instruction & 0x7ffu) != 0u)
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            switch (rd) {
            case 8u: cp_value = *cop0_badvaddr; break;
            case 12u: cp_value = *cop0_status; break;
            case 13u: cp_value = *cop0_cause; break;
            case 14u: cp_value = *cop0_epc; break;
            case 15u: cp_value = 0x00000002u; break; /* R3000A PRId */
            default: return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            }
            result = cp_value;
            write_result = true;
            is_load = true; /* MFC0 has the R3000A one-instruction transfer delay. */
        } else if (cop_rs == 4u) { /* MTC0 */
            if ((instruction & 0x7ffu) != 0u)
                return PSX_CPU_STEP_RESERVED_INSTRUCTION;
            switch (rd) {
            case 12u: *cop0_status = b; break;
            case 13u:
                /* Software may change IP0/IP1; hardware IP bits remain external. */
                *cop0_cause = (*cop0_cause & ~0x00000300u) |
                              (b & 0x00000300u);
                break;
            case 14u: *cop0_epc = b; break;
            default: return PSX_CPU_STEP_RESERVED_INSTRUCTION; /* BadVAddr and PRId are read-only. */
            }
        } else if (cop_rs == 16u && (instruction & 0x7ffu) == 0x10u) { /* RFE */
            *cop0_status = (*cop0_status & ~0x0000000fu) |
                           ((*cop0_status >> 2) & 0x0000000fu);
        } else {
            return PSX_CPU_STEP_RESERVED_INSTRUCTION;
        }
        break;
    }
    case 0x01u: { /* REGIMM: branches and conditional traps */
        bool negative = (a & 0x80000000u) != 0u;
        bool take_branch = false;
        bool is_branch = true;
        uint32_t immediate = sign_extend_16(imm);
        switch (rt) {
        case 0u:  take_branch = negative; break; /* BLTZ */
        case 1u:  take_branch = !negative; break; /* BGEZ */
        case 8u:  is_branch = false; if (signed_less(a, immediate) == 0u) return PSX_CPU_STEP_TRAP; break; /* TGEI */
        case 9u:  is_branch = false; if (a >= immediate) return PSX_CPU_STEP_TRAP; break; /* TGEIU */
        case 10u: is_branch = false; if (signed_less(a, immediate) != 0u) return PSX_CPU_STEP_TRAP; break; /* TLTI */
        case 11u: is_branch = false; if (a < immediate) return PSX_CPU_STEP_TRAP; break; /* TLTIU */
        case 12u: is_branch = false; if (a == immediate) return PSX_CPU_STEP_TRAP; break; /* TEQI */
        case 14u: is_branch = false; if (a != immediate) return PSX_CPU_STEP_TRAP; break; /* TNEI */
        case 16u: take_branch = negative; break; /* BLTZAL */
        case 17u: take_branch = !negative; break; /* BGEZAL */
        default: return PSX_CPU_STEP_RESERVED_INSTRUCTION;
        }
        if (!is_branch) break;
        control_transfer = true;
        if (take_branch) {
            branch = true;
            branch_target = old_pc + 4u + (immediate << 2);
        }
        /* R3000A link variants write RA whether or not the condition matches. */
        if (rt == 16u || rt == 17u) {
            rt = 31u;
            result = old_pc + 8u;
            write_result = true;
        }
        break;
    }
    case 0x02u:
    case 0x03u:
        control_transfer = true;
        branch = true;
        branch_target = ((old_pc + 4u) & 0xf0000000u) |
                        ((instruction & 0x03ffffffu) << 2);
        if (op == 0x03u) { rt = 31u; result = old_pc + 8u; write_result = true; }
        break;
    case 0x04u: case 0x05u: case 0x06u: case 0x07u:
        /* BLEZ/BGTZ encode rt as zero; other values are reserved. */
        if ((op == 0x06u || op == 0x07u) && rt != 0u)
            return PSX_CPU_STEP_RESERVED_INSTRUCTION;
        control_transfer = true;
        if ((op == 0x04u && a == b) || (op == 0x05u && a != b) ||
            (op == 0x06u && ((a & 0x80000000u) != 0u || a == 0u)) ||
            (op == 0x07u && ((a & 0x80000000u) == 0u && a != 0u))) {
            branch = true;
            branch_target = old_pc + 4u + (sign_extend_16(imm) << 2);
        }
        break;
    case 0x08u: { /* ADDI: stop rather than silently wrap until exceptions exist */
        uint32_t operand = sign_extend_16(imm);
        result = a + operand;
        if (((~(a ^ operand)) & (a ^ result) & 0x80000000u) != 0u)
            return PSX_CPU_STEP_OVERFLOW;
        write_result = true;
        break;
    }
    case 0x09u: result = a + sign_extend_16(imm); write_result = true; break;
    case 0x0au: result = signed_less(a, sign_extend_16(imm)); write_result = true; break;
    case 0x0bu: result = a < sign_extend_16(imm); write_result = true; break;
    case 0x0cu: result = a & (uint32_t)imm; write_result = true; break;
    case 0x0du: result = a | (uint32_t)imm; write_result = true; break;
    case 0x0eu: result = a ^ (uint32_t)imm; write_result = true; break;
    case 0x0fu: result = (uint32_t)imm << 16; write_result = true; break;
    case 0x20u: /* LB */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read8(bus, address, &byte_value)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        result = (byte_value & 0x80u) ? (0xffffff00u | byte_value) : byte_value;
        write_result = true; is_load = true; break;
    case 0x21u: /* LH */
        address = a + sign_extend_16(imm);
        if ((address & 1u) != 0u) { cpu->fault_address = address; return PSX_CPU_STEP_ADDRESS_ERROR_LOAD; }
        if (!psx_bus_read16(bus, address, &half_value)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        result = (half_value & 0x8000u) ? (0xffff0000u | half_value) : half_value;
        write_result = true; is_load = true; break;
    case 0x22u: /* LWL — Load Word Left, little-endian */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read32(bus, address & ~3u, &result)) {
            cpu->fault_address = address;
            return PSX_CPU_STEP_DATA_BUS_ERROR;
        }
        switch (address & 3u) {
        case 0u: result = (b & 0x00ffffffu) | (result << 24); break;
        case 1u: result = (b & 0x0000ffffu) | (result << 16); break;
        case 2u: result = (b & 0x000000ffu) | (result << 8); break;
        default: break;
        }
        write_result = true; is_load = true; is_merge_load = true; break;
    case 0x23u: /* LW */
        address = a + sign_extend_16(imm);
        if ((address & 3u) != 0u) { cpu->fault_address = address; return PSX_CPU_STEP_ADDRESS_ERROR_LOAD; }
        if (!psx_bus_read32(bus, address, &result)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        write_result = true; is_load = true; break;
    case 0x24u: /* LBU */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read8(bus, address, &byte_value)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        result = byte_value; write_result = true; is_load = true; break;
    case 0x25u: /* LHU */
        address = a + sign_extend_16(imm);
        if ((address & 1u) != 0u) { cpu->fault_address = address; return PSX_CPU_STEP_ADDRESS_ERROR_LOAD; }
        if (!psx_bus_read16(bus, address, &half_value)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        result = half_value; write_result = true; is_load = true; break;
    case 0x26u: /* LWR — Load Word Right, little-endian */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read32(bus, address & ~3u, &result)) {
            cpu->fault_address = address;
            return PSX_CPU_STEP_DATA_BUS_ERROR;
        }
        switch (address & 3u) {
        case 0u: break;
        case 1u: result = (b & 0xff000000u) | (result >> 8); break;
        case 2u: result = (b & 0xffff0000u) | (result >> 16); break;
        default: result = (b & 0xffffff00u) | (result >> 24); break;
        }
        write_result = true; is_load = true; is_merge_load = true; break;
    case 0x32u: /* LWC2 */
        if (!cop0_status || ((*cop0_status & 0x40000000u) == 0u) || !gte)
            return PSX_CPU_STEP_COPROCESSOR_UNUSABLE;
        address = a + sign_extend_16(imm);
        if ((address & 3u) != 0u) { cpu->fault_address = address; return PSX_CPU_STEP_ADDRESS_ERROR_LOAD; }
        if (!psx_bus_read32(bus, address, &result)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        if (!psx_gte_write_data(gte, (uint8_t)rt, result))
            return PSX_CPU_STEP_RESERVED_INSTRUCTION;
        break;
    case 0x28u: /* SB */
        address = a + sign_extend_16(imm);
        if (!psx_bus_write8(bus, address, (uint8_t)b)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        break;
    case 0x29u: /* SH */
        address = a + sign_extend_16(imm);
        if ((address & 1u) != 0u) { cpu->fault_address = address; return PSX_CPU_STEP_ADDRESS_ERROR_STORE; }
        if (!psx_bus_write16(bus, address, (uint16_t)b)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        break;
    case 0x2au: /* SWL — Store Word Left, little-endian */
    case 0x2eu: { /* SWR — Store Word Right, little-endian */
        unsigned offset, first_lane, last_lane, lane;
        address = a + sign_extend_16(imm);
        offset = address & 3u;
        if (op == 0x2au) {
            first_lane = 0u;
            last_lane = offset;
        } else {
            first_lane = offset;
            last_lane = 3u;
        }
        for (lane = first_lane; lane <= last_lane; ++lane) {
            unsigned register_lane = (op == 0x2au)
                ? (3u - offset + lane) : (lane - offset);
            uint8_t value = (uint8_t)(b >> (register_lane * 8u));
            if (!psx_bus_write8(bus, (address & ~3u) + lane, value)) {
                cpu->fault_address = address;
                return PSX_CPU_STEP_DATA_BUS_ERROR;
            }
        }
        break;
    }
    case 0x3au: /* SWC2 */
        if (!cop0_status || ((*cop0_status & 0x40000000u) == 0u) || !gte)
            return PSX_CPU_STEP_COPROCESSOR_UNUSABLE;
        address = a + sign_extend_16(imm);
        if ((address & 3u) != 0u) { cpu->fault_address = address; return PSX_CPU_STEP_ADDRESS_ERROR_STORE; }
        if (!psx_gte_read_data(gte, (uint8_t)rt, &result))
            return PSX_CPU_STEP_RESERVED_INSTRUCTION;
        if (!psx_bus_write32(bus, address, result)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        break;
    case 0x2bu: /* SW */
        address = a + sign_extend_16(imm);
        if ((address & 3u) != 0u) { cpu->fault_address = address; return PSX_CPU_STEP_ADDRESS_ERROR_STORE; }
        if (!psx_bus_write32(bus, address, b)) { cpu->fault_address = address; return PSX_CPU_STEP_DATA_BUS_ERROR; }
        break;
    default:
        return PSX_CPU_STEP_RESERVED_INSTRUCTION;
    }

    {
        unsigned current_register = (op == 0u) ? rd : rt;
        bool current_writes_gpr = write_result && current_register != 0u;

        /* Operand reads above see the old GPR value. Retire the preceding
         * load only after this instruction has executed. A newer non-load
         * write to the same register takes precedence. Back-to-back loads
         * retire the older result while queuing the newer result.
         */
        if (previous_load_pending && previous_load_register != 0u &&
            !(current_writes_gpr && !is_load &&
              current_register == previous_load_register))
            psx_cpu_write_gpr(cpu, previous_load_register,
                              previous_load_value);

        cpu->load_delay_pending = false;
        if (is_load) {
            if (current_register != 0u) {
                cpu->load_delay_register = (uint8_t)current_register;
                cpu->load_delay_value = result;
                cpu->load_delay_pending = true;
                cpu->load_delay_mergeable = is_merge_load;
            } else {
                cpu->load_delay_mergeable = false;
            }
        } else {
            cpu->load_delay_mergeable = false;
            if (write_result)
                psx_cpu_write_gpr(cpu, current_register, result);
        }
    }
    cpu->pc = sequential_pc;
    cpu->next_pc = branch ? branch_target : sequential_next;
    if (was_delay_slot) {
        cpu->delay_slot_pending = false;
    } else if (control_transfer) {
        cpu->delay_slot_pending = true;
        cpu->delay_branch_pc = old_pc;
    }
    cpu->gpr[0] = 0u;
    return PSX_CPU_STEP_OK;
}

PsxCpuStepResult psx_cpu_step_result_with_cop0(PsxCpu *cpu, PsxBus *bus,
                            uint32_t *status, uint32_t *cause,
                            uint32_t *epc, uint32_t *badvaddr)
{
    return psx_cpu_step_result_with_cop0_gte(cpu, bus, status, cause,
                                              epc, badvaddr, NULL);
}

bool psx_cpu_step_with_cop0(PsxCpu *cpu, PsxBus *bus,
                            uint32_t *status, uint32_t *cause,
                            uint32_t *epc, uint32_t *badvaddr)
{
    return psx_cpu_step_result_with_cop0(cpu, bus, status, cause, epc,
                                         badvaddr) == PSX_CPU_STEP_OK;
}

bool psx_cpu_step(PsxCpu *cpu, PsxBus *bus)
{
    return psx_cpu_step_with_cop0(cpu, bus, NULL, NULL, NULL, NULL);
}
