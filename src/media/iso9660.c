#include "dreampsx/iso9660.h"

#include <string.h>

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24u) | ((uint32_t)p[1] << 16u) |
           ((uint32_t)p[2] << 8u) | (uint32_t)p[3];
}

static bool both_endian32(const uint8_t *p, uint32_t *value)
{
    const uint32_t little = read_le32(p);
    const uint32_t big = read_be32(p + 4u);
    if (little != big) return false;
    *value = little;
    return true;
}

static bool range_valid(const PsxIso9660 *iso, uint32_t lba, uint32_t size)
{
    const uint64_t start = (uint64_t)lba * PSX_ISO9660_SECTOR_SIZE;
    return start <= iso->image_size &&
           (uint64_t)size <= (uint64_t)iso->image_size - start;
}

bool psx_iso9660_mount(PsxIso9660 *iso, const uint8_t *image, size_t size)
{
    const uint8_t *pvd;
    const uint8_t *root;
    uint32_t sectors, root_lba, root_size;
    if (!iso) return false;
    memset(iso, 0, sizeof(*iso));
    if (!image || size < 17u * PSX_ISO9660_SECTOR_SIZE ||
        (size % PSX_ISO9660_SECTOR_SIZE) != 0u)
        return false;
    pvd = image + 16u * PSX_ISO9660_SECTOR_SIZE;
    if (pvd[0] != 1u || memcmp(pvd + 1u, "CD001", 5u) != 0 ||
        pvd[6] != 1u ||
        !both_endian32(pvd + 80u, &sectors) ||
        sectors == 0u ||
        (uint64_t)sectors * PSX_ISO9660_SECTOR_SIZE > size)
        return false;
    root = pvd + 156u;
    if (root[0] < 34u || root[1] != 0u ||
        !both_endian32(root + 2u, &root_lba) ||
        !both_endian32(root + 10u, &root_size) ||
        (root[25] & 0x02u) == 0u ||
        !range_valid(&(PsxIso9660){.image_size = size}, root_lba, root_size))
        return false;
    iso->image = image;
    iso->image_size = size;
    iso->volume_sectors = sectors;
    iso->root_lba = root_lba;
    iso->root_size = root_size;
    iso->mounted = true;
    return true;
}

void psx_iso9660_unmount(PsxIso9660 *iso)
{
    if (iso) memset(iso, 0, sizeof(*iso));
}

static uint8_t ascii_upper(uint8_t c)
{
    return c >= 'a' && c <= 'z' ? (uint8_t)(c - ('a' - 'A')) : c;
}

static bool identifier_matches(const uint8_t *identifier, size_t identifier_size,
                               const char *name, size_t name_size)
{
    size_t base_size = identifier_size;
    size_t requested_size = name_size;
    size_t i;
    /* ISO-9660 file identifiers commonly carry a ;1 version suffix. */
    for (i = 0u; i < base_size; ++i) {
        if (identifier[i] == ';') { base_size = i; break; }
    }
    while (base_size > 0u && identifier[base_size - 1u] == '.') --base_size;
    while (requested_size > 0u && name[requested_size - 1u] == '.') --requested_size;
    if (base_size != requested_size) return false;
    for (i = 0u; i < base_size; ++i) {
        if (ascii_upper(identifier[i]) != ascii_upper((uint8_t)name[i]))
            return false;
    }
    return true;
}

static bool find_in_directory(const PsxIso9660 *iso, uint32_t dir_lba,
                              uint32_t dir_size, const char *name,
                              size_t name_size, PsxIso9660File *file)
{
    const size_t start = (size_t)dir_lba * PSX_ISO9660_SECTOR_SIZE;
    size_t offset = 0u;
    if (!range_valid(iso, dir_lba, dir_size)) return false;
    while (offset < dir_size) {
        const uint8_t *record = iso->image + start + offset;
        const uint8_t length = record[0];
        uint8_t id_length;
        uint32_t extent, data_size;
        if (length == 0u) {
            offset = ((offset / PSX_ISO9660_SECTOR_SIZE) + 1u) *
                     PSX_ISO9660_SECTOR_SIZE;
            continue;
        }
        if (length < 34u || (size_t)length > dir_size - offset)
            return false;
        id_length = record[32];
        if ((size_t)33u + id_length > length ||
            !both_endian32(record + 2u, &extent) ||
            !both_endian32(record + 10u, &data_size))
            return false;
        if (id_length != 1u || (record[33] != 0u && record[33] != 1u)) {
            if (identifier_matches(record + 33u, id_length, name, name_size)) {
                if (!range_valid(iso, extent, data_size)) return false;
                file->lba = extent;
                file->size = data_size;
                file->directory = (record[25] & 0x02u) != 0u;
                return true;
            }
        }
        offset += length;
    }
    return false;
}

bool psx_iso9660_lookup(const PsxIso9660 *iso, const char *path,
                        PsxIso9660File *file)
{
    uint32_t dir_lba, dir_size;
    const char *cursor;
    if (!iso || !iso->mounted || !path || !file || path[0] != '/')
        return false;
    dir_lba = iso->root_lba;
    dir_size = iso->root_size;
    cursor = path;
    while (*cursor == '/') ++cursor;
    if (*cursor == '\0') {
        file->lba = dir_lba;
        file->size = dir_size;
        file->directory = true;
        return true;
    }
    while (*cursor != '\0') {
        const char *component = cursor;
        size_t length;
        PsxIso9660File found;
        while (*cursor != '\0' && *cursor != '/') ++cursor;
        length = (size_t)(cursor - component);
        if (length == 0u || length >= PSX_ISO9660_PATH_MAX) return false;
        if (!find_in_directory(iso, dir_lba, dir_size, component, length,
                               &found))
            return false;
        while (*cursor == '/') ++cursor;
        if (*cursor != '\0') {
            if (!found.directory) return false;
            dir_lba = found.lba;
            dir_size = found.size;
        } else {
            *file = found;
            return true;
        }
    }
    return false;
}

bool psx_iso9660_read(const PsxIso9660 *iso, const PsxIso9660File *file,
                      uint32_t offset, void *destination, size_t length)
{
    uint64_t absolute;
    if (!iso || !iso->mounted || !file || (!destination && length != 0u) ||
        file->directory || offset > file->size ||
        length > (size_t)(file->size - offset))
        return false;
    absolute = (uint64_t)file->lba * PSX_ISO9660_SECTOR_SIZE + offset;
    if (absolute > iso->image_size ||
        length > iso->image_size - (size_t)absolute)
        return false;
    if (length != 0u)
        memcpy(destination, iso->image + (size_t)absolute, length);
    return true;
}
