#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dreampsx/kos_disc_file.h"
#include <kos/fs.h>

static uint8_t backing[4096];
static size_t cursor;
static int opened, closed, fail_seek, fail_open;
file_t fs_open(const char *path, int mode)
{
    assert(path != NULL);
    assert(mode == O_RDONLY);
    if (fail_open) return FILEHND_INVALID;
    opened++;
    cursor = 0u;
    return 3;
}
int fs_close(file_t handle)
{
    assert(handle == 3);
    closed++;
    return 0;
}
ssize_t fs_total(file_t handle)
{
    assert(handle == 3);
    return (ssize_t)sizeof(backing);
}
off_t fs_seek(file_t handle, off_t offset, int whence)
{
    assert(handle == 3);
    assert(whence == SEEK_SET);
    if (fail_seek || offset < 0 || (size_t)offset > sizeof(backing))
        return (off_t)-1;
    cursor = (size_t)offset;
    return offset;
}
ssize_t fs_read(file_t handle, void *buffer, size_t count)
{
    size_t available, amount;
    assert(handle == 3);
    if (cursor >= sizeof(backing)) return 0;
    available = sizeof(backing) - cursor;
    amount = count < available ? count : available;
    if (amount > 7u) amount = 7u; /* exercise short-read completion */
    memcpy(buffer, backing + cursor, amount);
    cursor += amount;
    return (ssize_t)amount;
}

int main(void)
{
    PsxKosDiscFile file;
    PsxDiscImage disc;
    uint8_t output[32];
    size_t i;

    memset(&file, 0, sizeof(file));
    memset(&disc, 0, sizeof(disc));
    for (i = 0u; i < sizeof(backing); ++i)
        backing[i] = (uint8_t)(i & 0xffu);

    assert(psx_kos_disc_file_open(&file, &disc, "/cd/game.bin",
                                  PSX_DISC_MODE1_2048));
    assert(file.opened && file.file_size == sizeof(backing));
    assert(disc.mounted && disc.source == NULL && disc.read_at != NULL);
    assert(psx_kos_disc_file_read_at(&file, 123u, output, sizeof(output)));
    for (i = 0u; i < sizeof(output); ++i)
        assert(output[i] == backing[123u + i]);

    /* A sector request is serviced via seek + complete reads, not a full
       image allocation. MODE1/2048 data begins at byte zero. */
    assert(psx_disc_read_sector(&disc, 1u, output));
    assert(output[0] == backing[2048u]);
    assert(output[31] == backing[2079u]);

    fail_seek = 1;
    assert(!psx_kos_disc_file_read_at(&file, 0u, output, sizeof(output)));
    fail_seek = 0;
    assert(!psx_kos_disc_file_read_at(&file, sizeof(backing) - 2u,
                                      output, sizeof(output)));

    psx_kos_disc_file_close(&file, &disc);
    assert(!file.opened && !disc.mounted && closed == 1);

    fail_open = 1;
    assert(!psx_kos_disc_file_open(&file, &disc, "/missing.bin",
                                   PSX_DISC_MODE1_2048));
    assert(!file.opened && !disc.mounted);
    fail_open = 0;

    /* A non-sector-aligned file is closed and rejected during mount. */
    /* The test VFS reports the fixed 4096-byte aligned file above. */
    assert(opened == 1);
    puts("KOS disc file tests passed");
    return 0;
}
