#ifndef DREAMPSX_KOS_DISC_FILE_H
#define DREAMPSX_KOS_DISC_FILE_H

#include <stdbool.h>
#include <stddef.h>
#include "dreampsx/disc_image.h"

typedef struct {
    int file_handle;
    size_t file_size;
    bool opened;
} PsxKosDiscFile;

/* Opens a file through the KallistiOS VFS and mounts it as a random-access
 * single-track image. The file remains open until psx_kos_disc_file_close().
 * Do not read/seek this handle from another thread while the disc is mounted.
 */
bool psx_kos_disc_file_open(PsxKosDiscFile *file, PsxDiscImage *disc,
                            const char *path, PsxDiscTrackMode mode);
/* Loads a bounded single-track CUE sheet and opens its relative BIN/ISO file. */
bool psx_kos_disc_file_open_cue(PsxKosDiscFile *file, PsxDiscImage *disc,
                                const char *cue_path);
void psx_kos_disc_file_close(PsxKosDiscFile *file, PsxDiscImage *disc);

/* Exposed for the generic disc reader callback contract and host tests. */
bool psx_kos_disc_file_read_at(void *context, size_t offset,
                               uint8_t *destination, size_t size);

#endif
