#include "dreampsx/kos_disc_file.h"

#include <fcntl.h>
#include <limits.h>
#include <kos/fs.h>

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
