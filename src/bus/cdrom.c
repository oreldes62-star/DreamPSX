#include "dreampsx/cdrom.h"
#include <string.h>

static uint8_t cdrom_status(const PsxCdrom *cdrom)
{
    uint8_t status = (uint8_t)(cdrom->index & 3u);
    if (cdrom->parameter_count == 0u) status |= 1u << 3u;
    if (cdrom->parameter_count < PSX_CDROM_FIFO_CAPACITY)
        status |= 1u << 4u;
    if (cdrom->response_count != 0u) status |= 1u << 5u;
    if ((cdrom->request & 0x80u) != 0u && cdrom->data_count != 0u)
        status |= 1u << 6u;
    if ((cdrom->drive_status & 0x20u) != 0u) status |= 1u << 7u;
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
    if (interrupt >= 1u && interrupt <= 5u)
        cdrom->interrupt_flags |= (uint8_t)(1u << (interrupt - 1u));
}

static bool bcd_valid(uint8_t value, unsigned maximum)
{
    const unsigned high = (unsigned)(value >> 4u);
    const unsigned low = (unsigned)(value & 0x0fu);
    return high <= 9u && low <= 9u && high * 10u + low <= maximum;
}

static void command_error(PsxCdrom *cdrom)
{
    cdrom->drive_status |= 0x01u;
    (void)push_response(cdrom, cdrom->drive_status);
    command_complete(cdrom, 5u);
}

static bool load_current_sector(PsxCdrom *cdrom)
{
    const unsigned minute = (unsigned)(cdrom->location[0] >> 4u) * 10u +
                            (unsigned)(cdrom->location[0] & 0x0fu);
    const unsigned second = (unsigned)(cdrom->location[1] >> 4u) * 10u +
                            (unsigned)(cdrom->location[1] & 0x0fu);
    const unsigned frame = (unsigned)(cdrom->location[2] >> 4u) * 10u +
                           (unsigned)(cdrom->location[2] & 0x0fu);
    const unsigned absolute_frame = minute * 60u * 75u +
                                    second * 75u + frame;
    size_t offset;
    if (!cdrom->iso_data || !bcd_valid(cdrom->location[0], 99u) ||
        !bcd_valid(cdrom->location[1], 59u) ||
        !bcd_valid(cdrom->location[2], 74u) || absolute_frame < 150u)
        return false;
    offset = (size_t)(absolute_frame - 150u) * PSX_CDROM_SECTOR_SIZE;
    if (offset > cdrom->iso_size ||
        cdrom->iso_size - offset < PSX_CDROM_SECTOR_SIZE)
        return false;
    memcpy(cdrom->sector_data, cdrom->iso_data + offset,
           PSX_CDROM_SECTOR_SIZE);
    cdrom->data_index = 0u;
    cdrom->data_count = PSX_CDROM_SECTOR_SIZE;
    cdrom->drive_status &= (uint8_t)~0x01u;
    cdrom->drive_status |= 0x20u;
    command_complete(cdrom, 1u);
    return true;
}

static void increment_location(PsxCdrom *cdrom)
{
    unsigned minute = (unsigned)(cdrom->location[0] >> 4u) * 10u +
                      (unsigned)(cdrom->location[0] & 0x0fu);
    unsigned second = (unsigned)(cdrom->location[1] >> 4u) * 10u +
                      (unsigned)(cdrom->location[1] & 0x0fu);
    unsigned frame = (unsigned)(cdrom->location[2] >> 4u) * 10u +
                     (unsigned)(cdrom->location[2] & 0x0fu);
    if (++frame >= 75u) {
        frame = 0u;
        if (++second >= 60u) {
            second = 0u;
            if (minute < 99u) ++minute;
        }
    }
    cdrom->location[0] = (uint8_t)(((minute / 10u) << 4u) | (minute % 10u));
    cdrom->location[1] = (uint8_t)(((second / 10u) << 4u) | (second % 10u));
    cdrom->location[2] = (uint8_t)(((frame / 10u) << 4u) | (frame % 10u));
}

static void execute_command(PsxCdrom *cdrom, uint8_t command)
{
    switch (command) {
    case 0x01u: /* Getstat */
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x02u: /* Setloc: three BCD MSF bytes. */
        if (cdrom->parameter_count < 3u ||
            !pop_parameter(cdrom, &cdrom->location[0]) ||
            !pop_parameter(cdrom, &cdrom->location[1]) ||
            !pop_parameter(cdrom, &cdrom->location[2]) ||
            !bcd_valid(cdrom->location[0], 99u) ||
            !bcd_valid(cdrom->location[1], 59u) ||
            !bcd_valid(cdrom->location[2], 74u)) {
            command_error(cdrom);
            break;
        }
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x06u: /* ReadN starts a continuous sector stream. */
        cdrom->read_active = true;
        if (!load_current_sector(cdrom)) {
            cdrom->read_active = false;
            command_error(cdrom);
            break;
        }
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x08u: /* Stop: acknowledgement now, mechanical completion later. */
    case 0x09u: { /* Pause */
        uint32_t delay;
        if (cdrom->pending_command != 0u) {
            command_error(cdrom);
            break;
        }
        if (command == 0x09u) {
            delay = (cdrom->mode & 0x80u) != 0u
                ? PSX_CDROM_PAUSE_DOUBLE_CYCLES
                : PSX_CDROM_PAUSE_NORMAL_CYCLES;
        } else {
            delay = (cdrom->mode & 0x80u) != 0u
                ? PSX_CDROM_STOP_DOUBLE_CYCLES
                : PSX_CDROM_STOP_NORMAL_CYCLES;
        }
        cdrom->pending_command = command;
        cdrom->command_cycles_remaining = delay;
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    }
    case 0x0au: /* Init */
        cdrom->drive_status = 0u;
        cdrom->request = 0u;
        cdrom->data_index = 0u;
        cdrom->data_count = 0u;
        cdrom->read_active = false;
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x0eu: /* Setmode */
        if (cdrom->parameter_count < 1u ||
            !pop_parameter(cdrom, &cdrom->mode)) {
            command_error(cdrom);
            break;
        }
        (void)push_response(cdrom, cdrom->drive_status);
        command_complete(cdrom, 3u);
        break;
    case 0x11u: /* GetlocL requires sector-header decoding, not implemented. */
    default:
        command_error(cdrom);
        break;
    }
}

bool psx_cdrom_init(PsxCdrom *cdrom)
{
    if (!cdrom) return false;
    memset(cdrom, 0, sizeof(*cdrom));
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
    cdrom->data_index = 0u;
    cdrom->data_count = 0u;
    cdrom->read_active = false;
    cdrom->mode = 0u;
    cdrom->pending_command = 0u;
    cdrom->command_cycles_remaining = 0u;
    cdrom->location[0] = 0u;
    cdrom->location[1] = 0u;
    cdrom->location[2] = 0u;
}

bool psx_cdrom_mount_iso(PsxCdrom *cdrom, const uint8_t *image, size_t size)
{
    if (!cdrom || !cdrom->initialized || !image || size == 0u ||
        (size % PSX_CDROM_SECTOR_SIZE) != 0u) return false;
    cdrom->iso_data = image;
    cdrom->iso_size = size;
    cdrom->drive_status &= (uint8_t)~0x01u;
    return true;
}

void psx_cdrom_unmount(PsxCdrom *cdrom)
{
    if (!cdrom || !cdrom->initialized) return;
    cdrom->iso_data = NULL;
    cdrom->iso_size = 0u;
    cdrom->data_index = 0u;
    cdrom->data_count = 0u;
    cdrom->read_active = false;
    cdrom->drive_status |= 0x01u;
    cdrom->drive_status &= (uint8_t)~0x20u;
    cdrom->interrupt_flags &= (uint8_t)~0x01u;
}

bool psx_cdrom_advance_sector(PsxCdrom *cdrom)
{
    if (!cdrom || !cdrom->initialized || !cdrom->read_active ||
        cdrom->data_count != 0u)
        return false;
    increment_location(cdrom);
    if (!load_current_sector(cdrom)) {
        cdrom->read_active = false;
        cdrom->drive_status |= 0x01u;
        cdrom->drive_status &= (uint8_t)~0x20u;
        command_complete(cdrom, 5u);
        return false;
    }
    return true;
}

uint32_t psx_cdrom_sector_period_cycles(const PsxCdrom *cdrom)
{
    if (!cdrom || !cdrom->initialized) return 0u;
    return (cdrom->mode & 0x80u) != 0u
        ? PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES
        : PSX_CDROM_NORMAL_SPEED_SECTOR_CYCLES;
}

void psx_cdrom_advance_cycles(PsxCdrom *cdrom, uint32_t cycles)
{
    if (!cdrom || !cdrom->initialized ||
        cdrom->pending_command == 0u)
        return;
    if (cycles < cdrom->command_cycles_remaining) {
        cdrom->command_cycles_remaining -= cycles;
        return;
    }

    cdrom->command_cycles_remaining = 0u;
    if (cdrom->pending_command == 0x09u) {
        cdrom->read_active = false;
        cdrom->drive_status &= (uint8_t)~0x20u;
        cdrom->drive_status |= 0x02u;
    } else if (cdrom->pending_command == 0x08u) {
        cdrom->read_active = false;
        cdrom->drive_status &= (uint8_t)~0x22u;
        cdrom->data_index = 0u;
        cdrom->data_count = 0u;
        cdrom->request = 0u;
    }
    cdrom->pending_command = 0u;
    (void)push_response(cdrom, cdrom->drive_status);
    command_complete(cdrom, 2u);
}

bool psx_cdrom_read_data32(PsxCdrom *cdrom, uint32_t *value)
{
    uint32_t word = 0u;
    if (!cdrom || !cdrom->initialized || !value ||
        (cdrom->request & 0x80u) == 0u || cdrom->data_count < 4u)
        return false;

    for (unsigned byte = 0u; byte < 4u; ++byte) {
        word |= (uint32_t)cdrom->sector_data[cdrom->data_index++] <<
                (byte * 8u);
        --cdrom->data_count;
    }
    if (cdrom->data_count == 0u)
        cdrom->drive_status &= (uint8_t)~0x20u;
    *value = word;
    return true;
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
        if (cdrom->index == 0u && cdrom->response_count != 0u) {
            *value = cdrom->responses[cdrom->response_head];
            cdrom->response_head = (uint8_t)(
                (cdrom->response_head + 1u) % PSX_CDROM_FIFO_CAPACITY);
            --cdrom->response_count;
        } else {
            *value = 0u;
        }
        return true;
    case 2u:
        if (cdrom->index == 1u) {
            *value = cdrom->interrupt_enable;
        } else if (cdrom->index == 0u) {
            if ((cdrom->request & 0x80u) != 0u &&
                cdrom->data_count != 0u) {
                *value = cdrom->sector_data[cdrom->data_index++];
                --cdrom->data_count;
                if (cdrom->data_count == 0u)
                    cdrom->drive_status &= (uint8_t)~0x20u;
            } else {
                *value = 0u;
            }
        } else {
            *value = 0u;
        }
        return true;
    case 3u:
        *value = cdrom->index == 1u
            ? (uint8_t)(cdrom->interrupt_flags | 0xe0u) : 0u;
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
        if (cdrom->index == 0u) execute_command(cdrom, value);
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
