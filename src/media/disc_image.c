#include "dreampsx/disc_image.h"

#include <string.h>

static bool equal_ci(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a++, cb = *b++;
        if (ca >= 'a' && ca <= 'z') ca = (char)(ca - ('a' - 'A'));
        if (cb >= 'a' && cb <= 'z') cb = (char)(cb - ('a' - 'A'));
        if (ca != cb) return false;
    }
    return *a == '\0' && *b == '\0';
}

static char *skip_spaces(char *p)
{
    while (*p == ' ' || *p == '\t') ++p;
    return p;
}

static bool parse_two_digits(const char **cursor, unsigned *value)
{
    const char *p = *cursor;
    if (p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9')
        return false;
    *value = (unsigned)(p[0] - '0') * 10u + (unsigned)(p[1] - '0');
    *cursor = p + 2;
    return true;
}

static bool parse_index_time(char *p, size_t *frames)
{
    unsigned minute, second, frame;
    const char *cursor = p;
    if (!parse_two_digits(&cursor, &minute) || *cursor++ != ':' ||
        !parse_two_digits(&cursor, &second) || *cursor++ != ':' ||
        !parse_two_digits(&cursor, &frame) || *cursor != '\0' ||
        second >= 60u || frame >= 75u)
        return false;
    *frames = ((size_t)minute * 60u + second) * 75u + frame;
    return true;
}

static bool parse_track_mode(char *p, PsxDiscTrackMode *mode,
                             uint16_t *sector_size,
                             uint16_t *data_offset)
{
    if (equal_ci(p, "MODE1/2048")) {
        *mode = PSX_DISC_MODE1_2048;
        *sector_size = 2048u;
        *data_offset = 0u;
    } else if (equal_ci(p, "MODE1/2352")) {
        *mode = PSX_DISC_MODE1_2352;
        *sector_size = 2352u;
        *data_offset = 16u;
    } else if (equal_ci(p, "MODE2/2352")) {
        *mode = PSX_DISC_MODE2_2352;
        *sector_size = 2352u;
        *data_offset = 24u;
    } else {
        return false;
    }
    return true;
}

bool psx_disc_mount_cue(PsxDiscImage *disc, const char *cue_text,
                        size_t cue_size, const uint8_t *bin_data,
                        size_t bin_size)
{
    size_t position = 0u, index_frames = 0u;
    uint16_t sector_size = 0u, data_offset = 0u;
    PsxDiscTrackMode mode = PSX_DISC_MODE1_2048;
    bool file_seen = false, track_seen = false, index_seen = false;
    if (!disc) return false;
    memset(disc, 0, sizeof(*disc));
    if (!cue_text || cue_size == 0u || !bin_data || bin_size == 0u)
        return false;

    while (position < cue_size) {
        char line[PSX_DISC_CUE_LINE_MAX];
        size_t length = 0u;
        char *p, *command, *argument;
        while (position < cue_size && cue_text[position] != '\n') {
            if (length + 1u >= sizeof(line)) return false;
            line[length++] = cue_text[position++];
        }
        if (position < cue_size && cue_text[position] == '\n') ++position;
        if (length != 0u && line[length - 1u] == '\r') --length;
        line[length] = '\0';
        p = skip_spaces(line);
        if (*p == '\0' || *p == ';') continue;
        command = p;
        while (*p && *p != ' ' && *p != '\t') ++p;
        if (*p) *p++ = '\0';
        argument = skip_spaces(p);

        if (equal_ci(command, "FILE")) {
            char *end;
            if (file_seen || *argument != '"') return false;
            end = strchr(argument + 1, '"');
            if (!end || end == argument + 1) return false;
            argument = skip_spaces(end + 1);
            if (!equal_ci(argument, "BINARY")) return false;
            file_seen = true;
        } else if (equal_ci(command, "TRACK")) {
            unsigned track_number;
            char *mode_text;
            const char *cursor = argument;
            if (!file_seen || track_seen ||
                !parse_two_digits(&cursor, &track_number) ||
                track_number != 1u || (*cursor != ' ' && *cursor != '\t'))
                return false;
            mode_text = skip_spaces((char *)cursor);
            if (!parse_track_mode(mode_text, &mode, &sector_size,
                                  &data_offset))
                return false;
            track_seen = true;
        } else if (equal_ci(command, "INDEX")) {
            unsigned index_number;
            char *time_text;
            const char *cursor = argument;
            if (!track_seen || index_seen ||
                !parse_two_digits(&cursor, &index_number) ||
                index_number != 1u || (*cursor != ' ' && *cursor != '\t'))
                return false;
            time_text = skip_spaces((char *)cursor);
            if (!parse_index_time(time_text, &index_frames)) return false;
            index_seen = true;
        }
    }

    if (!file_seen || !track_seen || !index_seen ||
        bin_size % sector_size != 0u ||
        index_frames >= bin_size / sector_size)
        return false;

    disc->source = bin_data;
    disc->source_size = bin_size;
    disc->first_sector = index_frames;
    disc->sector_count = bin_size / sector_size - index_frames;
    disc->source_sector_size = sector_size;
    disc->user_data_offset = data_offset;
    disc->mode = mode;
    disc->mounted = true;
    return true;
}

void psx_disc_unmount(PsxDiscImage *disc)
{
    if (disc) memset(disc, 0, sizeof(*disc));
}

bool psx_disc_read_sector(const PsxDiscImage *disc, size_t lba,
                          uint8_t destination[PSX_DISC_SECTOR_DATA_SIZE])
{
    size_t source_sector, offset;
    const uint8_t *sector;
    if (!disc || !disc->mounted || !destination || !disc->source ||
        lba >= disc->sector_count ||
        disc->first_sector > SIZE_MAX - lba)
        return false;

    /* Validate the public descriptor before doing any pointer arithmetic.
     * Mounted images are caller-owned, so reject corrupted/inconsistent
     * metadata rather than trusting offsets copied into the descriptor. */
    switch (disc->mode) {
    case PSX_DISC_MODE1_2048:
        if (disc->source_sector_size != 2048u ||
            disc->user_data_offset != 0u)
            return false;
        break;
    case PSX_DISC_MODE1_2352:
        if (disc->source_sector_size != 2352u ||
            disc->user_data_offset != 16u)
            return false;
        break;
    case PSX_DISC_MODE2_2352:
        if (disc->source_sector_size != 2352u ||
            disc->user_data_offset != 24u)
            return false;
        break;
    default:
        return false;
    }
    if (disc->source_size % disc->source_sector_size != 0u ||
        disc->first_sector >=
            disc->source_size / disc->source_sector_size ||
        disc->sector_count >
            disc->source_size / disc->source_sector_size -
                disc->first_sector)
        return false;

    source_sector = disc->first_sector + lba;
    if (source_sector > SIZE_MAX / disc->source_sector_size)
        return false;
    offset = source_sector * disc->source_sector_size;
    if (offset > disc->source_size ||
        disc->source_size - offset < disc->source_sector_size)
        return false;
    sector = disc->source + offset;

    if (disc->mode == PSX_DISC_MODE1_2048) {
        memcpy(destination, sector, PSX_DISC_SECTOR_DATA_SIZE);
        return true;
    }
    if (sector[0] != 0x00u || sector[11] != 0x00u ||
        sector[1] != 0xffu || sector[10] != 0xffu)
        return false;
    for (unsigned i = 2u; i < 10u; ++i)
        if (sector[i] != 0xffu) return false;
    if ((disc->mode == PSX_DISC_MODE1_2352 && sector[15] != 1u) ||
        (disc->mode == PSX_DISC_MODE2_2352 && sector[15] != 2u))
        return false;
    if (disc->mode == PSX_DISC_MODE2_2352) {
        if (memcmp(sector + 16u, sector + 20u, 4u) != 0 ||
            (sector[18] & 0x20u) != 0u)
            return false; /* XA Form 2 does not contain 2048 user bytes. */
    }
    memcpy(destination, sector + disc->user_data_offset,
           PSX_DISC_SECTOR_DATA_SIZE);
    return true;
}


bool psx_disc_read_sector_form2(
    const PsxDiscImage *disc, size_t lba,
    uint8_t destination[PSX_DISC_SECTOR_FORM2_DATA_SIZE])
{
    size_t source_sector, offset;
    const uint8_t *sector;

    if (!disc || !disc->mounted || !destination || !disc->source ||
        disc->mode != PSX_DISC_MODE2_2352 ||
        disc->source_sector_size != 2352u ||
        disc->user_data_offset != 24u ||
        lba >= disc->sector_count ||
        disc->first_sector > SIZE_MAX - lba ||
        disc->source_size % 2352u != 0u ||
        disc->first_sector >= disc->source_size / 2352u ||
        disc->sector_count >
            disc->source_size / 2352u - disc->first_sector)
        return false;

    source_sector = disc->first_sector + lba;
    if (source_sector > SIZE_MAX / 2352u)
        return false;
    offset = source_sector * 2352u;
    if (offset > disc->source_size ||
        disc->source_size - offset < 2352u)
        return false;
    sector = disc->source + offset;

    if (sector[0] != 0x00u || sector[11] != 0x00u ||
        sector[1] != 0xffu || sector[10] != 0xffu)
        return false;
    for (unsigned i = 2u; i < 10u; ++i)
        if (sector[i] != 0xffu) return false;
    if (sector[15] != 2u ||
        memcmp(sector + 16u, sector + 20u, 4u) != 0 ||
        (sector[18] & 0x20u) == 0u)
        return false;

    memcpy(destination, sector + 24u, PSX_DISC_SECTOR_FORM2_DATA_SIZE);
    return true;
}
