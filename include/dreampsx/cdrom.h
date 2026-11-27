#ifndef DREAMPSX_CDROM_H
#define DREAMPSX_CDROM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "dreampsx/disc_image.h"

#define PSX_CDROM_BASE_ADDR 0x1f801800u
#define PSX_CDROM_FIFO_CAPACITY 16u
#define PSX_CDROM_SECTOR_SIZE 2048u
/* PS1 system cycles per sector at double speed (33,868,800 / 150). */
#define PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES 225792u
#define PSX_CDROM_NORMAL_SPEED_SECTOR_CYCLES 451584u
/* Measured command-completion averages from PSX-SPX, in PS1 system cycles. */
#define PSX_CDROM_PAUSE_NORMAL_CYCLES 0x021181cu
#define PSX_CDROM_PAUSE_DOUBLE_CYCLES 0x010bd93u
#define PSX_CDROM_STOP_NORMAL_CYCLES 0x0d38acau
#define PSX_CDROM_STOP_DOUBLE_CYCLES 0x18a6076u

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
    uint8_t sector_data[PSX_CDROM_SECTOR_SIZE];
    uint16_t data_index;
    uint16_t data_count;
    bool read_active;
    uint8_t mode;
    uint8_t pending_command;
    uint32_t command_cycles_remaining;
    PsxDiscImage disc;
    const uint8_t *iso_data;
    size_t iso_size;
    uint8_t location[3];
    bool initialized;
} PsxCdrom;

bool psx_cdrom_init(PsxCdrom *cdrom);
bool psx_cdrom_mount_iso(PsxCdrom *cdrom, const uint8_t *image, size_t size);
/* Mounts a CUE-described BIN track without copying the image. */
bool psx_cdrom_mount_cue(PsxCdrom *cdrom, const char *cue_text,
                         size_t cue_size, const uint8_t *bin_data,
                         size_t bin_size);
void psx_cdrom_unmount(PsxCdrom *cdrom);
/* Deliver the next sector of an active ReadN stream once the previous
 * sector has been fully consumed. The caller supplies CD-sector timing. */
bool psx_cdrom_advance_sector(PsxCdrom *cdrom);
/* Read one little-endian word from the data FIFO for CD-ROM DMA. */
bool psx_cdrom_read_data32(PsxCdrom *cdrom, uint32_t *value);
/* Advance asynchronous command completion using PS1 system cycles. */
void psx_cdrom_advance_cycles(PsxCdrom *cdrom, uint32_t cycles);
uint32_t psx_cdrom_sector_period_cycles(const PsxCdrom *cdrom);
void psx_cdrom_reset(PsxCdrom *cdrom);
bool psx_cdrom_read8(PsxCdrom *cdrom, uint32_t address, uint8_t *value);
bool psx_cdrom_write8(PsxCdrom *cdrom, uint32_t address, uint8_t value);
bool psx_cdrom_irq_output(const PsxCdrom *cdrom);

#endif
