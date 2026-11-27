#include "dreampsx/cdrom.h"

static uint8_t cdrom_status(const PsxCdrom *cdrom)
{
    uint8_t status = (uint8_t)(cdrom->index & 3u);
    if (cdrom->parameter_count == 0u) status |= 1u << 3u;
    if (cdrom->parameter_count < PSX_CDROM_FIFO_CAPACITY)
        status |= 1u << 4u;
    if (cdrom->response_count != 0u) status |= 1u << 5u;
    if ((cdrom->request & 0x80u) != 0u) status |= 1u << 6u;
    return status;
}

static bool push_response(PsxCdrom *cdrom, uint8_t value)
{
    uint8_t tail;
    if (cdrom->response_count >= PSX_CDROM_FIFO_CAPACITY) return false;
    tail = (uint8_t)((cdrom->response_head + cdrom->response_count) %
                     PSX_CDROM_FIFO_CAPACITY);
    cdrom->responses[tail] = value;
    ++cdrom->response_count;
    return true;
}

static bool pop_parameter(PsxCdrom *cdrom, uint8_t *value)
{
    if (cdrom->parameter_count == 0u) return false;
    *value = cdrom->parameters[cdrom->parameter_head];
    cdrom->parameter_head = (uint8_t)((cdrom->parameter_head + 1u) %
                                      PSX_CDROM_FIFO_CAPACITY);
    --cdrom->parameter_count;
    return true;
}

static void command_complete(PsxCdrom *cdrom, uint8_t interrupt)
{
    cdrom->interrupt_flags = interrupt >= 1u && interrupt <= 5u
        ? (uint8_t)(1u << (interrupt - 1u)) : 0u;
}

static void execute_command(PsxCdrom *cdrom, uint8_t command)
{
    uint8_t parameter;
    switch (command) {
    case 0x01u: /* Getstat */
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x02u: /* Setloc: three BCD bytes, retained for ReadN/Seek later. */
        if (cdrom->parameter_count < 3u ||
            !pop_parameter(cdrom, &cdrom->location[0]) ||
            !pop_parameter(cdrom, &cdrom->location[1]) ||
            !pop_parameter(cdrom, &cdrom->location[2])) {
            cdrom->drive_status |= 0x01u;
            (void)push_response(cdrom, cdrom->drive_status);
            command_complete(cdrom, 5u);
            break;
        }
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x0au: /* Init */
        cdrom->drive_status = 0u;
        cdrom->request = 0u;
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x11u: /* GetlocL: location header is not yet backed by a sector. */
        cdrom->drive_status |= 0x01u;
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 5u);
        break;
    default:
        /* No media/sector engine exists yet: fail explicitly, never invent data. */
        cdrom->drive_status |= 0x01u;
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 5u);
        break;
    }
}

bool psx_cdrom_init(PsxCdrom *cdrom)
{
    if (!cdrom) return false;
    psx_cdrom_reset(cdrom);
    cdrom->initialized = true;
    return true;
}

void psx_cdrom_reset(PsxCdrom *cdrom)
{
    if (!cdrom) return;
    cdrom->index = 0u;
    cdrom->parameter_head = 0u;
    cdrom->parameter_count = 0u;
    cdrom->response_head = 0u;
    cdrom->response_count = 0u;
    cdrom->interrupt_enable = 0u;
    cdrom->interrupt_flags = 0u;
    cdrom->request = 0u;
    cdrom->drive_status = 0u;
    cdrom->location[0] = 0u;
    cdrom->location[1] = 0u;
    cdrom->location[2] = 0u;
}

bool psx_cdrom_irq_output(const PsxCdrom *cdrom)
{
    return cdrom && cdrom->initialized &&
           (cdrom->interrupt_flags & cdrom->interrupt_enable & 0x1fu) != 0u;
}

bool psx_cdrom_read8(PsxCdrom *cdrom, uint32_t address, uint8_t *value)
{
    unsigned reg;
    if (!cdrom || !cdrom->initialized || !value ||
        address < PSX_CDROM_BASE_ADDR ||
        address >= PSX_CDROM_BASE_ADDR + 4u) return false;
    reg = (unsigned)(address - PSX_CDROM_BASE_ADDR);
    switch (reg) {
    case 0u:
        *value = cdrom_status(cdrom);
        return true;
    case 1u:
        if (cdrom->index == 0u) {
            if (cdrom->response_count == 0u) {
                *value = 0u;
            } else {
                *value = cdrom->responses[cdrom->response_head];
                cdrom->response_head = (uint8_t)(
                    (cdrom->response_head + 1u) % PSX_CDROM_FIFO_CAPACITY);
                --cdrom->response_count;
            }
        } else {
            *value = 0u;
        }
        return true;
    case 2u:
        *value = cdrom->index == 1u ? cdrom->interrupt_enable : 0u;
        return true;
    case 3u:
        *value = cdrom->index == 1u
            ? (uint8_t)(cdrom->interrupt_flags | 0xe0u)
            : 0u;
        return true;
    default:
        return false;
    }
}

bool psx_cdrom_write8(PsxCdrom *cdrom, uint32_t address, uint8_t value)
{
    unsigned reg;
    if (!cdrom || !cdrom->initialized ||
        address < PSX_CDROM_BASE_ADDR ||
        address >= PSX_CDROM_BASE_ADDR + 4u) return false;
    reg = (unsigned)(address - PSX_CDROM_BASE_ADDR);
    switch (reg) {
    case 0u:
        cdrom->index = (uint8_t)(value & 3u);
        return true;
    case 1u:
        if (cdrom->index == 0u) {
            execute_command(cdrom, value);
        }
        return true;
    case 2u:
        if (cdrom->index == 0u) {
            uint8_t tail;
            if (cdrom->parameter_count >= PSX_CDROM_FIFO_CAPACITY)
                return false;
            tail = (uint8_t)((cdrom->parameter_head +
                              cdrom->parameter_count) %
                             PSX_CDROM_FIFO_CAPACITY);
            cdrom->parameters[tail] = value;
            ++cdrom->parameter_count;
        } else if (cdrom->index == 1u) {
            cdrom->interrupt_enable = (uint8_t)(value & 0x1fu);
        } else if (cdrom->index == 2u) {
            cdrom->request = value;
        }
        return true;
    case 3u:
        if (cdrom->index == 0u) {
            cdrom->interrupt_flags &= (uint8_t)~(value & 0x1fu);
            if ((value & 0x40u) != 0u) {
                cdrom->parameter_head = 0u;
                cdrom->parameter_count = 0u;
            }
        }
        return true;
    default:
        return false;
    }
}
