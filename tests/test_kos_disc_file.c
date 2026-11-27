#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dreampsx/kos_disc_file.h"
#include <kos/fs.h>

static uint8_t backing[4096];
static const char cue_text[] =
    "FILE \"game.bin\" BINARY\r\n"
    "  TRACK 01 MODE1/2048\r\n"
    "    INDEX 01 00:00:01\r\n";
static size_t cursor;
static int opened, closed, fail_seek, fail_open;
static ssize_t reported_size = (ssize_t)sizeof(backing);
ssize_t fs_total(file_t handle)
{
    if (handle == 4) return (ssize_t)(sizeof(cue_text) - 1u);
    assert(handle == 3);
    return reported_size;
}
file_t fs_open(const char *path, int mode)
{
    assert(path != NULL);
    assert(mode == O_RDONLY);
    if (fail_open) return FILEHND_INVALID;
    ++opened;
    cursor = 0u;
    if (strcmp(path, "/cd/game.cue") == 0) return 4;
    assert(strcmp(path, "/cd/game.bin") == 0 ||
           strcmp(path, "/cd/bad.bin") == 0);
    return 3;
}
int fs_close(file_t handle)
{
    assert(handle == 3 || handle == 4);
    ++closed;
    return 0;
}
off_t fs_seek(file_t handle, off_t offset, int whence)
{
    assert(handle == 3 || handle == 4);
    assert(whence == SEEK_SET);
    if (fail_seek || offset < 0 ||
        (handle == 3 && (size_t)offset > sizeof(backing)) ||
        (handle == 4 && (size_t)offset > sizeof(cue_text) - 1u))
        return (off_t)-1;
    cursor = (size_t)offset;
    return offset;
}
ssize_t fs_read(file_t handle, void *buffer, size_t count)
{
    const uint8_t *source;
    size_t source_size, available, amount;
    assert(handle == 3 || handle == 4);
    if (handle == 4) {
        source = (const uint8_t *)cue_text;
        source_size = sizeof(cue_text) - 1u;
    } else {
        source = backing;
        source_size = sizeof(backing);
    }
    if (cursor >= source_size) return 0;
    available = source_size - cursor;
    amount = count < available ? count : available;
    if (amount > 7u) amount = 7u; /* exercise short-read completion */
    memcpy(buffer, source + cursor, amount);
    cursor += amount;
    return (ssize_t)amount;
}

int main(void)
{
    PsxKosDiscFile file;
    PsxDiscImage disc;
    uint8_t output[PSX_DISC_SECTOR_DATA_SIZE];
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

    assert(psx_disc_read_sector(&disc, 1u, output));
    assert(output[0] == backing[2048u]);
    assert(output[PSX_DISC_SECTOR_DATA_SIZE - 1u] == backing[4095u]);

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

    reported_size = (ssize_t)sizeof(backing) - 1;
    assert(!psx_kos_disc_file_open(&file, &disc, "/cd/bad.bin",
                                   PSX_DISC_MODE1_2048));
    assert(!file.opened && !disc.mounted && closed == 2);
    assert(opened == 2);

    /* CUE is read in short chunks, its BIN path is resolved relative to
       /cd/game.cue, and INDEX 01 skips the first 2048-byte sector. */
    reported_size = (ssize_t)sizeof(backing);
    assert(psx_kos_disc_file_open_cue(&file, &disc, "/cd/game.cue"));
    assert(file.opened && disc.mounted);
    assert(disc.first_sector == 1u && disc.sector_count == 1u);
    assert(disc.mode == PSX_DISC_MODE1_2048);
    assert(psx_disc_read_sector(&disc, 0u, output));
    assert(output[0] == backing[2048u]);
    assert(output[PSX_DISC_SECTOR_DATA_SIZE - 1u] == backing[4095u]);
    psx_kos_disc_file_close(&file, &disc);
    assert(!file.opened && !disc.mounted);
    puts("KOS disc file tests passed");
    return 0;
}
