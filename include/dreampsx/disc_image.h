#ifndef DREAMPSX_DISC_IMAGE_H
#define DREAMPSX_DISC_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSX_DISC_SECTOR_DATA_SIZE 2048u
#define PSX_DISC_SECTOR_FORM2_DATA_SIZE 2324u
#define PSX_DISC_CUE_LINE_MAX 256u

typedef enum {
    PSX_DISC_MODE1_2048 = 0,
    PSX_DISC_MODE1_2352,
    PSX_DISC_MODE2_2352
} PsxDiscTrackMode;

typedef struct {
    uint16_t sample_rate_hz;
    uint8_t channels;
    uint8_t bits_per_sample;
    bool emphasis;
} PsxDiscXaAudioFormat;

typedef struct {
    uint8_t file;
    uint8_t channel;
    uint8_t submode;
    uint8_t coding;
} PsxDiscXaSubheader;

typedef bool (*PsxDiscReadAt)(void *context, size_t offset, uint8_t *destination, size_t size);

typedef struct {
    const uint8_t *source;
    PsxDiscReadAt read_at;
    void *read_context;
    size_t source_size;
    size_t first_sector;
    size_t sector_count;
    uint16_t source_sector_size;
    uint16_t user_data_offset;
    PsxDiscTrackMode mode;
    bool mounted;
} PsxDiscImage;

/* Parses a single-file, single-track CUE sheet against caller-owned BIN
 * bytes. Track 01 and INDEX 01 are required; only BINARY data tracks in
 * MODE1/2048, MODE1/2352 or MODE2/2352 are accepted. */
bool psx_disc_mount_cue(PsxDiscImage *disc, const char *cue_text,
                        size_t cue_size, const uint8_t *bin_data,
                        size_t bin_size);
bool psx_disc_mount_reader(PsxDiscImage *disc, size_t source_size, PsxDiscTrackMode mode, PsxDiscReadAt read_at, void *read_context);
void psx_disc_unmount(PsxDiscImage *disc);
/* Reads one 2048-byte user-data sector relative to INDEX 01. */
bool psx_disc_read_sector(const PsxDiscImage *disc, size_t lba,
                          uint8_t destination[PSX_DISC_SECTOR_DATA_SIZE]);
/* Reads the 2324-byte payload of a validated MODE2/XA Form 2 sector. */
bool psx_disc_read_sector_form2(
    const PsxDiscImage *disc, size_t lba,
    uint8_t destination[PSX_DISC_SECTOR_FORM2_DATA_SIZE]);
bool psx_disc_read_sector_form2_ex(
    const PsxDiscImage *disc, size_t lba,
    uint8_t destination[PSX_DISC_SECTOR_FORM2_DATA_SIZE],
    PsxDiscXaSubheader *subheader);
/* Decodes XA audio flags and coding byte; rejects non-audio/unsupported modes. */
bool psx_disc_xa_audio_format(const PsxDiscXaSubheader *subheader,
                              PsxDiscXaAudioFormat *format);

#endif
