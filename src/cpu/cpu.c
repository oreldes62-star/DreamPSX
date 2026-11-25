#include "dreampsx/cpu.h"
#include <string.h>

static uint32_t sign_extend_16(uint32_t value)
{
    return (value & 0x8000u) ? (value | 0xffff0000u) : (value & 0xffffu);
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
    uint32_t instruction;
    uint32_t old_pc, sequential_pc, sequential_next;
    unsigned op, rs, rt, rd, shamt, funct;
    uint32_t a, b, result = 0u, address;
    uint16_t imm;
    bool write_result = false;
    bool branch = false;
    uint32_t branch_target = 0u;

    if (!psx_cpu_fetch_instruction(cpu, bus, &instruction)) return false;

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
    case 0x00u: /* SPECIAL */
        switch (funct) {
        case 0x00u: result = b << shamt; write_result = true; break; /* SLL */
        case 0x02u: result = b >> shamt; write_result = true; break; /* SRL */
        case 0x03u: result = (uint32_t)((int32_t)b >> shamt); write_result = true; break; /* SRA */
        case 0x08u: branch = true; branch_target = a; break; /* JR */
        case 0x21u: result = a + b; write_result = true; break; /* ADDU */
        case 0x23u: result = a - b; write_result = true; break; /* SUBU */
        case 0x24u: result = a & b; write_result = true; break;
        case 0x25u: result = a | b; write_result = true; break;
        case 0x26u: result = a ^ b; write_result = true; break;
        case 0x27u: result = ~(a | b); write_result = true; break;
        case 0x2au: result = (int32_t)a < (int32_t)b; write_result = true; break;
        case 0x2bu: result = a < b; write_result = true; break;
        default: return false;
        }
        break;
    case 0x02u: /* J */
    case 0x03u: /* JAL */
        branch = true;
        branch_target = ((old_pc + 4u) & 0xf0000000u) |
                        ((instruction & 0x03ffffffu) << 2);
        if (op == 0x03u) psx_cpu_write_gpr(cpu, 31u, old_pc + 8u);
        break;
    case 0x04u: /* BEQ */
    case 0x05u: /* BNE */
    case 0x06u: /* BLEZ */
    case 0x07u: /* BGTZ */
        if ((op == 0x04u && a == b) || (op == 0x05u && a != b) ||
            (op == 0x06u && (int32_t)a <= 0) ||
            (op == 0x07u && (int32_t)a > 0)) {
            branch = true;
            branch_target = old_pc + 4u + (sign_extend_16(imm) << 2);
        }
        break;
    case 0x09u: result = a + sign_extend_16(imm); write_result = true; break; /* ADDIU */
    case 0x0cu: result = a & (uint32_t)imm; write_result = true; break;
    case 0x0du: result = a | (uint32_t)imm; write_result = true; break;
    case 0x0eu: result = a ^ (uint32_t)imm; write_result = true; break;
    case 0x0fu: result = (uint32_t)imm << 16; write_result = true; break; /* LUI */
    case 0x23u: /* LW */
        address = a + sign_extend_16(imm);
        if (!psx_bus_read32(bus, address, &result)) return false;
        write_result = true;
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
