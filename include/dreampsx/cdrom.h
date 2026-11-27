#ifndef DREAMPSX_CDROM_H
#define DREAMPSX_CDROM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "dreampsx/disc_image.h"
#include "dreampsx/xa_adpcm.h"

#define PSX_CDROM_BASE_ADDR 0x1f801800u
#define PSX_CDROM_FIFO_CAPACITY 16u
#define PSX_CDROM_SECTOR_SIZE 2048u
#define PSX_CDROM_MAX_SECTOR_SIZE PSX_DISC_SECTOR_FORM2_DATA_SIZE
#define PSX_CDROM_XA_PCM_QUEUE_FRAMES 8192u
#define PSX_CDROM_SYSTEM_CLOCK_HZ 33868800u
#define PSX_CDROM_XA_PCM_STORAGE_FRAMES \
    (PSX_CDROM_XA_PCM_QUEUE_FRAMES + PSX_XA_ADPCM_MAX_SECTOR_SAMPLES)
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
    uint8_t sector_data[PSX_CDROM_MAX_SECTOR_SIZE];
    uint16_t data_index;
    uint16_t data_count;
    bool read_active;
    uint8_t mode;
    uint8_t pending_command;
    uint32_t command_cycles_remaining;
    PsxDiscImage disc;
    PsxDiscXaSubheader xa_subheader;
    bool xa_subheader_valid;
    uint8_t xa_filter_file;
    uint8_t xa_filter_channel;
    PsxXaAdpcmState xa_decoder_state[2];
    int16_t xa_pcm_left[PSX_CDROM_XA_PCM_STORAGE_FRAMES];
    int16_t xa_pcm_right[PSX_CDROM_XA_PCM_STORAGE_FRAMES];
    uint16_t xa_pcm_sample_rate;
    uint16_t xa_pcm_head;
    uint16_t xa_pcm_count;
    uint32_t xa_dropped_sectors;
    uint64_t xa_audio_clock_phase;
    uint16_t xa_audio_clock_rate_hz;
    uint32_t xa_audio_due_frames;
    uint32_t xa_audio_underrun_frames;
    const uint8_t *iso_data;
    size_t iso_size;
    uint8_t location[3];
    bool initialized;
} PsxCdrom;

bool psx_cdrom_init(PsxCdrom *cdrom);
bool psx_cdrom_mount_iso(PsxCdrom *cdrom, const uint8_t *image, size_t size);
bool psx_cdrom_mount_disc(PsxCdrom *cdrom, const PsxDiscImage *disc);
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
/* Returns the format only when the currently loaded sector is XA audio. */
bool psx_cdrom_get_xa_audio_format(const PsxCdrom *cdrom,
                                   PsxDiscXaAudioFormat *format);
/* True when the current XA audio sector matches the programmed Setfilter. */
bool psx_cdrom_xa_sector_matches_filter(const PsxCdrom *cdrom);
/* Drain decoded XA PCM frames. Both outputs are required; sample_rate_hz
 * reports the rate shared by the currently queued frames. */
/* Advance the deterministic XA playback clock using PS1 system cycles. */
void psx_cdrom_advance_xa_audio_cycles(PsxCdrom *cdrom, uint32_t cycles);
/* Drain only frames whose playback time has elapsed. */
bool psx_cdrom_read_xa_audio_timed(PsxCdrom *cdrom, int16_t *left,
                                   int16_t *right, size_t max_frames,
                                   size_t *frames_read,
                                   uint16_t *sample_rate_hz);
bool psx_cdrom_read_xa_audio(PsxCdrom *cdrom, int16_t *left, int16_t *right,
                             size_t max_frames, size_t *frames_read,
                             uint16_t *sample_rate_hz);

#endif
