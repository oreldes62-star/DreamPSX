#ifndef DREAMPSX_ISO9660_H
#define DREAMPSX_ISO9660_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "dreampsx/disc_image.h"

#define PSX_ISO9660_SECTOR_SIZE 2048u
#define PSX_ISO9660_PATH_MAX 256u

typedef struct {
    const uint8_t *image;
    size_t image_size;
    uint32_t volume_sectors;
    uint32_t root_lba;
    uint32_t root_size;
    PsxDiscImage disc;
    bool mounted;
} PsxIso9660;

typedef struct {
    uint32_t lba;
    uint32_t size;
    bool directory;
} PsxIso9660File;

/* Mounts an in-memory ISO-9660 image by validating its primary volume
 * descriptor and root directory record. The image remains caller-owned. */
bool psx_iso9660_mount(PsxIso9660 *iso, const uint8_t *image, size_t size);
/* Mounts ISO-9660 through a CUE/BIN sector reader without expanding the image. */
bool psx_iso9660_mount_disc(PsxIso9660 *iso, const PsxDiscImage *disc);
void psx_iso9660_unmount(PsxIso9660 *iso);
/* Resolves an absolute ISO path such as /SYSTEM.CNF; ;version suffixes
 * are optional. Only single-extent files and directories are supported. */
bool psx_iso9660_lookup(const PsxIso9660 *iso, const char *path,
                        PsxIso9660File *file);
/* Reads a bounded byte range from a resolved file into caller storage. */
bool psx_iso9660_read(const PsxIso9660 *iso, const PsxIso9660File *file,
                      uint32_t offset, void *destination, size_t length);

#endif
