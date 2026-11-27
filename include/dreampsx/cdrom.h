#ifndef DREAMPSX_CDROM_H
#define DREAMPSX_CDROM_H

#include <stdbool.h>
#include <stdint.h>

#define PSX_CDROM_BASE_ADDR 0x1f801800u
#define PSX_CDROM_FIFO_CAPACITY 16u

typedef struct {
    uint8_t index;
    uint8_t parameters[PSX_CDROM_FIFO_CAPACITY];
    uint8_t parameter_head;
    uint8_t parameter_count;
    uint8_t responses[PSX_CDROM_FIFO_CAPACITY];
    uint8_t response_head;
    uint8_t response_count;
    uint8_t interrupt_enable;
    uint8_t interrupt_flags;
    uint8_t request;
    uint8_t drive_status;
    uint8_t location[3];
    bool initialized;
} PsxCdrom;

bool psx_cdrom_init(PsxCdrom *cdrom);
void psx_cdrom_reset(PsxCdrom *cdrom);
bool psx_cdrom_read8(PsxCdrom *cdrom, uint32_t address, uint8_t *value);
bool psx_cdrom_write8(PsxCdrom *cdrom, uint32_t address, uint8_t value);
bool psx_cdrom_irq_output(const PsxCdrom *cdrom);

#endif
