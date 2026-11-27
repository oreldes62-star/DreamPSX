#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dreampsx/iso9660.h"

static void put_both32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
    p[4] = (uint8_t)(value >> 24u);
    p[5] = (uint8_t)(value >> 16u);
    p[6] = (uint8_t)(value >> 8u);
    p[7] = (uint8_t)value;
}

static uint8_t *add_record(uint8_t *p, uint32_t lba, uint32_t size,
                           uint8_t flags, const uint8_t *name,
                           uint8_t name_size)
{
    uint8_t length = (uint8_t)(33u + name_size + ((name_size & 1u) == 0u));
    memset(p, 0, length);
    p[0] = length;
    put_both32(p + 2u, lba);
    put_both32(p + 10u, size);
    p[25] = flags;
    p[28] = 1u; p[29] = 0u; p[30] = 0u; p[31] = 1u;
    p[32] = name_size;
    memcpy(p + 33u, name, name_size);
    return p + length;
}

int main(void)
{
    static uint8_t image[24u * PSX_ISO9660_SECTOR_SIZE];
    const uint8_t dot = 0u, dotdot = 1u;
    const uint8_t system_name[] = "SYSTEM.CNF;1";
    const uint8_t dir_name[] = "GAME";
    const uint8_t exe_name[] = "SLUS_123.45;1";
    PsxIso9660 iso;
    PsxIso9660File file;
    uint8_t output[16] = {0};
    uint8_t *pvd = image + 16u * PSX_ISO9660_SECTOR_SIZE;
    uint8_t *root = image + 20u * PSX_ISO9660_SECTOR_SIZE;
    uint8_t *game = image + 21u * PSX_ISO9660_SECTOR_SIZE;
    uint8_t *p;

    pvd[0] = 1u; memcpy(pvd + 1u, "CD001", 5u); pvd[6] = 1u;
    put_both32(pvd + 80u, 24u);
    (void)add_record(pvd + 156u, 20u, PSX_ISO9660_SECTOR_SIZE, 2u,
                     &dot, 1u);
    pvd[156u + 32u] = 1u; pvd[156u + 33u] = 0u;
    p = add_record(root, 20u, PSX_ISO9660_SECTOR_SIZE, 2u, &dot, 1u);
    p = add_record(p, 20u, PSX_ISO9660_SECTOR_SIZE, 2u, &dotdot, 1u);
    p = add_record(p, 22u, 12u, 0u, system_name,
                   (uint8_t)(sizeof(system_name) - 1u));
    (void)add_record(p, 21u, PSX_ISO9660_SECTOR_SIZE, 2u, dir_name,
                     (uint8_t)(sizeof(dir_name) - 1u));
    p = add_record(game, 21u, PSX_ISO9660_SECTOR_SIZE, 2u, &dot, 1u);
    p = add_record(p, 20u, PSX_ISO9660_SECTOR_SIZE, 2u, &dotdot, 1u);
    (void)add_record(p, 23u, 12u, 0u, exe_name,
                     (uint8_t)(sizeof(exe_name) - 1u));
    memcpy(image + 22u * PSX_ISO9660_SECTOR_SIZE, "BOOT=cdrom", 10u);
    memcpy(image + 23u * PSX_ISO9660_SECTOR_SIZE, "PS-X EXE DATA", 12u);

    assert(psx_iso9660_mount(&iso, image, sizeof(image)));
    assert(iso.volume_sectors == 24u);
    assert(psx_iso9660_lookup(&iso, "/SYSTEM.CNF", &file));
    assert(!file.directory && file.lba == 22u && file.size == 12u);
    assert(psx_iso9660_read(&iso, &file, 0u, output, 10u));
    assert(memcmp(output, "BOOT=cdrom", 10u) == 0);
    assert(!psx_iso9660_read(&iso, &file, 8u, output, 8u));
    assert(psx_iso9660_lookup(&iso, "/game/slus_123.45", &file));
    assert(file.lba == 23u && !file.directory);
    assert(psx_iso9660_read(&iso, &file, 0u, output, sizeof(output)));
    assert(memcmp(output, "PS-X EXE DATA", 12u) == 0);
    assert(psx_iso9660_lookup(&iso, "/", &file) && file.directory);
    assert(!psx_iso9660_lookup(&iso, "/MISSING.BIN", &file));
    assert(!psx_iso9660_lookup(&iso, "SYSTEM.CNF", &file));

    pvd[1] = 'X';
    assert(!psx_iso9660_mount(&iso, image, sizeof(image)));
    psx_iso9660_unmount(&iso);
    assert(!iso.mounted);
    puts("iso9660 tests passed");
    return 0;
}
