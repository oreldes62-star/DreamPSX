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

bool psx_cpu_step(PsxCpu *cpu, PsxBus *bus)
{
    uint32_t instruction, old_pc, sequential_pc, sequential_next;
    unsigned op, rs, rt, rd, shamt, funct;
    uint32_t a, b, result = 0u, address;
    uint16_t imm;
    bool write_result = false, branch = false;
    uint32_t branch_target = 0u;
    uint8_t byte_value;
    uint16_t half_value;

    if (!cpu || !cpu->initialized ||
        !psx_cpu_fetch_instruction(cpu, bus, &instruction)) return false;

    old_pc = cpu->pc;
    sequential_pc = cpu->next_pc;
    sequential_next = cpu->next_pc + 4u;
    op = instruction >> 26;
    rs = (instruction >> 21) & 31u;
    rt = (instruction >> 16) & 31u;
    rd = (instruction >> 11) & 31u;
    shamt = (instruction >> 6) & 31u;
    funct = instruction & 63u;
    imm = (uint16_t)instruction;
    a = psx_cpu_read_gpr(cpu, rs);
    b = psx_cpu_read_gpr(cpu, rt);

    switch (op) {
    case 0x00u:
        switch (funct) {
        case 0x00u: result = b << shamt; write_result = true; break;
        case 0x02u: result = b >> shamt; write_result = true; break;
        case 0x03u: result = arithmetic_shift_right(b, shamt); write_result = true; break;
        case 0x08u: branch = true; branch_target = a; break; /* JR */
        case 0x09u: /* JALR */
            result = old_pc + 8u;
            psx_cpu_write_gpr(cpu, rd, result);
            branch = true; branch_target = a;
            break;
        case 0x10u: result = cpu->hi; write_result = true; break; /* MFHI */
        case 0x11u: cpu->hi = a; break; /* MTHI */
        case 0x12u: result = cpu->lo; write_result = true; break; /* MFLO */
        case 0x13u: cpu->lo = a; break; /* MTLO */
        case 0x18u: { /* MULT */
            int64_t product = signed32(a) * signed32(b);
            uint64_t bits = (uint64_t)product;
            cpu->lo = (uint32_t)bits;
            cpu->hi = (uint32_t)(bits >> 32);
            break;
        }
        case 0x19u: { /* MULTU */
            uint64_t product = (uint64_t)a * (uint64_t)b;
            cpu->lo = (uint32_t)product;
            cpu->hi = (uint32_t)(product >> 32);
            break;
        }
        case 0x1au: /* DIV */
            if (b == 0u) {
                cpu->lo = (a & 0x80000000u) ? 1u : 0xffffffffu;
                cpu->hi = a;
            } else if (a == 0x80000000u && b == 0xffffffffu) {
                cpu->lo = 0x80000000u; cpu->hi = 0u;
            } else {
                int64_t sa = signed32(a), sb = signed32(b);
                cpu->lo = (uint32_t)(sa / sb);
                cpu->hi = (uint32_t)(sa % sb);
            }
            break;
        case 0x1bu: /* DIVU */
            if (b == 0u) { cpu->lo = 0xffffffffu; cpu->hi = a; }
            else { cpu->lo = a / b; cpu->hi = a % b; }
            break;
        case 0x21u: result = a + b; write_result = true; break;
        case 0x23u: result = a - b; write_result = true; break;
        case 0x24u: result = a & b; write_result = true; break;
        case 0x25u: result = a | b; write_result = true; break;
        case 0x26u: result = a ^ b; write_result = true; break;
        case 0x27u: result = ~(a | b); write_result = true; break;
        case 0x2au: result = signed_less(a, b); write_result = true; break;
        case 0x2bu: result = a < b; write_result = true; break;
        default: return false;
        }
        break;
    case 0x02u:
    case 0x03u:
        branch = true;
        branch_target = ((old_pc + 4u) & 0xf0000000u) |
                        ((instruction & 0x03ffffffu) << 2);
        if (op == 0x03u) psx_cpu_write_gpr(cpu, 31u, old_pc + 8u);
        break;
    case 0x04u: case 0x05u: case 0x06u: case 0x07u:
        if ((op == 0x04u && a == b) || (op == 0x05u && a != b) ||
            (op == 0x06u && ((a & 0x80000000u) != 0u || a == 0u)) ||
            (op == 0x07u && ((a & 0x80000000u) == 0u && a != 0u))) {
            branch = true;
            branch_target = old_pc + 4u + (sign_extend_16(imm) << 2);
        }
        break;
    case 0x08u: result = a + sign_extend_16(imm); write_result = true; break; /* ADDI: overflow exception pending */
    case 0x09u: result = a + sign_extend_16(imm); write_result = true; break;
    case 0x0au: result = signed_less(a, sign_extend_16(imm)); write_result = true; break;
    case 0x0bu: result = a < sign_extend_16(imm); write_result = true; break;
    case 0x0cu: result = a & (uint32_t)imm; write_result = true; break;
    case 0x0du: result = a | (uint32_t)imm; write_result = true; break;
    case 0x0eu: result = a ^ (uint32_t)imm; write_result = true; break;
    case 0x0fu: result = (uint32_t)imm << 16; write_result = true; break;
    case 0x20u: /* LB */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read8(bus, address, &byte_value)) return false;
        result = (byte_value & 0x80u) ? (0xffffff00u | byte_value) : byte_value;
        write_result = true; break;
    case 0x21u: /* LH */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read16(bus, address, &half_value)) return false;
        result = (half_value & 0x8000u) ? (0xffff0000u | half_value) : half_value;
        write_result = true; break;
    case 0x23u: /* LW */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read32(bus, address, &result)) return false;
        write_result = true; break;
    case 0x24u: /* LBU */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read8(bus, address, &byte_value)) return false;
        result = byte_value; write_result = true; break;
    case 0x25u: /* LHU */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read16(bus, address, &half_value)) return false;
        result = half_value; write_result = true; break;
    case 0x28u: /* SB */
        address = a + sign_extend_16(imm);
        if (!psx_bus_write8(bus, address, (uint8_t)b)) return false;
        break;
    case 0x29u: /* SH */
        address = a + sign_extend_16(imm);
        if (!psx_bus_write16(bus, address, (uint16_t)b)) return false;
        break;
    case 0x2bu: /* SW */
        address = a + sign_extend_16(imm);
        if (!psx_bus_write32(bus, address, b)) return false;
        break;
    default:
        return false;
    }

    if (write_result) psx_cpu_write_gpr(cpu, (op == 0u) ? rd : rt, result);
    cpu->pc = sequential_pc;
    cpu->next_pc = branch ? branch_target : sequential_next;
    cpu->gpr[0] = 0u;
    return true;
}
