#include "dreampsx/kos_disc_file.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <kos/fs.h>

#define PSX_KOS_CUE_MAX_SIZE 4096u
#define PSX_KOS_CUE_LINE_MAX 256u

static char *cue_skip_spaces(char *p)
{
    while (*p == ' ' || *p == '\t') ++p;
    return p;
}

static bool cue_equal_ci(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a++, cb = *b++;
        if (ca >= 'a' && ca <= 'z') ca = (char)(ca - ('a' - 'A'));
        if (cb >= 'a' && cb <= 'z') cb = (char)(cb - ('a' - 'A'));
        if (ca != cb) return false;
    }
    return *a == '\0' && *b == '\0';
}

static bool cue_parse_digits(const char **cursor, unsigned *value)
{
    const char *p = *cursor;
    if (p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9')
        return false;
    *value = (unsigned)(p[0] - '0') * 10u + (unsigned)(p[1] - '0');
    *cursor = p + 2;
    return true;
}

static bool cue_parse_time(char *p, size_t *frames)
{
    unsigned minute, second, frame;
    const char *cursor = p;
    if (!cue_parse_digits(&cursor, &minute) || *cursor++ != ':' ||
        !cue_parse_digits(&cursor, &second) || *cursor++ != ':' ||
        !cue_parse_digits(&cursor, &frame) || *cursor != '\0' ||
        second >= 60u || frame >= 75u)
        return false;
    *frames = ((size_t)minute * 60u + second) * 75u + frame;
    return true;
}

static bool cue_parse_mode(char *p, PsxDiscTrackMode *mode)
{
    if (cue_equal_ci(p, "MODE1/2048")) *mode = PSX_DISC_MODE1_2048;
    else if (cue_equal_ci(p, "MODE1/2352")) *mode = PSX_DISC_MODE1_2352;
    else if (cue_equal_ci(p, "MODE2/2352")) *mode = PSX_DISC_MODE2_2352;
    else return false;
    return true;
}

static bool cue_parse_metadata(const char *text, size_t size,
                               char *filename, size_t filename_capacity,
                               PsxDiscTrackMode *mode, size_t *index_frames)
{
    size_t position = 0u;
    bool file_seen = false, track_seen = false, index_seen = false;
    if (!text || size == 0u || !filename || filename_capacity == 0u ||
        !mode || !index_frames) return false;
    while (position < size) {
        char line[PSX_KOS_CUE_LINE_MAX];
        size_t length = 0u;
        char *p, *command, *argument;
        while (position < size && text[position] != '\n') {
            if (length + 1u >= sizeof(line)) return false;
            line[length++] = text[position++];
        }
        if (position < size) ++position;
        if (length && line[length - 1u] == '\r') --length;
        line[length] = '\0';
        p = cue_skip_spaces(line);
        if (*p == '\0' || *p == ';') continue;
        command = p;
        while (*p && *p != ' ' && *p != '\t') ++p;
        if (*p) *p++ = '\0';
        argument = cue_skip_spaces(p);
        if (cue_equal_ci(command, "FILE")) {
            char *end;
            size_t name_length;
            if (file_seen || *argument != '"') return false;
            end = strchr(argument + 1, '"');
            if (!end || end == argument + 1) return false;
            name_length = (size_t)(end - (argument + 1));
            if (name_length >= filename_capacity) return false;
            memcpy(filename, argument + 1, name_length);
            filename[name_length] = '\0';
            if (!cue_equal_ci(cue_skip_spaces(end + 1), "BINARY")) return false;
            file_seen = true;
        } else if (cue_equal_ci(command, "TRACK")) {
            unsigned number;
            const char *cursor = argument;
            if (!file_seen || track_seen || !cue_parse_digits(&cursor, &number) ||
                number != 1u || (*cursor != ' ' && *cursor != '\t') ||
                !cue_parse_mode(cue_skip_spaces((char *)cursor), mode)) return false;
            track_seen = true;
        } else if (cue_equal_ci(command, "INDEX")) {
            unsigned number;
            const char *cursor = argument;
            if (!track_seen || index_seen || !cue_parse_digits(&cursor, &number) ||
                number != 1u || (*cursor != ' ' && *cursor != '\t') ||
                !cue_parse_time(cue_skip_spaces((char *)cursor), index_frames)) return false;
            index_seen = true;
        }
    }
    return file_seen && track_seen && index_seen;
}

static bool cue_resolve_path(const char *cue_path, const char *filename,
                             char *resolved, size_t capacity)
{
    size_t directory_length = 0u, name_length;
    const char *slash;
    if (!cue_path || !filename || !resolved || capacity == 0u || filename[0] == '\0')
        return false;
    name_length = strlen(filename);
    if (filename[0] == '/') {
        if (name_length >= capacity) return false;
        memcpy(resolved, filename, name_length + 1u);
        return true;
    }
    slash = strrchr(cue_path, '/');
    if (slash) directory_length = (size_t)(slash - cue_path) + 1u;
    if (directory_length > capacity || name_length >= capacity - directory_length)
        return false;
    memcpy(resolved, cue_path, directory_length);
    memcpy(resolved + directory_length, filename, name_length + 1u);
    return true;
}

bool psx_kos_disc_file_read_at(void *context, size_t offset,
                               uint8_t *destination, size_t size)
{
    PsxKosDiscFile *file = (PsxKosDiscFile *)context;
    size_t total = 0u;
    if (!file || !file->opened || !destination || size == 0u ||
        offset > file->file_size || size > file->file_size - offset ||
        offset > (size_t)INT32_MAX)
        return false;
    if (fs_seek(file->file_handle, (off_t)offset, SEEK_SET) != (off_t)offset)
        return false;
    while (total < size) {
        ssize_t count = fs_read(file->file_handle, destination + total,
                                size - total);
        if (count <= 0 || (size_t)count > size - total)
            return false;
        total += (size_t)count;
    }
    return true;
}

bool psx_kos_disc_file_open(PsxKosDiscFile *file, PsxDiscImage *disc,
                            const char *path, PsxDiscTrackMode mode)
{
    file_t handle;
    ssize_t file_size;
    if (!file || !disc || !path || path[0] == '\0')
        return false;
    file->file_handle = (int)FILEHND_INVALID;
    file->file_size = 0u;
    file->opened = false;
    psx_disc_unmount(disc);
    handle = fs_open(path, O_RDONLY);
    if (handle == FILEHND_INVALID)
        return false;
    file_size = fs_total(handle);
    if (file_size <= 0) {
        fs_close(handle);
        return false;
    }
    file->file_handle = (int)handle;
    file->file_size = (size_t)file_size;
    file->opened = true;
    if (!psx_disc_mount_reader(disc, file->file_size, mode,
                               psx_kos_disc_file_read_at, file)) {
        fs_close(handle);
        file->file_handle = (int)FILEHND_INVALID;
        file->file_size = 0u;
        file->opened = false;
        return false;
    }
    return true;
}

bool psx_kos_disc_file_open_cue(PsxKosDiscFile *file, PsxDiscImage *disc,
                                const char *cue_path)
{
    file_t cue_handle;
    ssize_t cue_size;
    char cue_text[PSX_KOS_CUE_MAX_SIZE + 1u];
    char filename[PSX_KOS_CUE_LINE_MAX];
    char image_path[PSX_KOS_CUE_LINE_MAX * 2u];
    PsxDiscTrackMode mode = PSX_DISC_MODE1_2048;
    size_t index_frames = 0u, total = 0u;

    if (!file || !disc || !cue_path || cue_path[0] == '\0')
        return false;
    psx_disc_unmount(disc);
    cue_handle = fs_open(cue_path, O_RDONLY);
    if (cue_handle == FILEHND_INVALID)
        return false;
    cue_size = fs_total(cue_handle);
    if (cue_size <= 0 || (size_t)cue_size > PSX_KOS_CUE_MAX_SIZE ||
        fs_seek(cue_handle, 0, SEEK_SET) != 0) {
        fs_close(cue_handle);
        return false;
    }
    while (total < (size_t)cue_size) {
        ssize_t count = fs_read(cue_handle, cue_text + total,
                                (size_t)cue_size - total);
        if (count <= 0 || (size_t)count > (size_t)cue_size - total) {
            fs_close(cue_handle);
            return false;
        }
        total += (size_t)count;
    }
    fs_close(cue_handle);
    cue_text[total] = '\0';

    if (!cue_parse_metadata(cue_text, total, filename, sizeof(filename),
                            &mode, &index_frames) ||
        !cue_resolve_path(cue_path, filename, image_path, sizeof(image_path)))
        return false;
    if (!psx_kos_disc_file_open(file, disc, image_path, mode))
        return false;
    if (index_frames >= disc->sector_count) {
        psx_kos_disc_file_close(file, disc);
        return false;
    }
    disc->first_sector = index_frames;
    disc->sector_count -= index_frames;
    return true;
}

void psx_kos_disc_file_close(PsxKosDiscFile *file, PsxDiscImage *disc)
{
    if (disc)
        psx_disc_unmount(disc);
    if (!file)
        return;
    if (file->opened)
        fs_close((file_t)file->file_handle);
    file->file_handle = (int)FILEHND_INVALID;
    file->file_size = 0u;
    file->opened = false;
}
