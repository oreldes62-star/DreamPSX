#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dreampsx/disc_image.h"

static void make_raw(uint8_t *sector, uint8_t mode, uint8_t seed)
{
    memset(sector, 0, 2352u);
    for (unsigned i = 1u; i <= 10u; ++i) sector[i] = 0xffu;
    sector[15] = mode;
    if (mode == 2u) {
        sector[16] = 1u; sector[17] = 2u;
        sector[18] = 0u; sector[19] = 4u;
        memcpy(sector + 20u, sector + 16u, 4u);
    }
    for (unsigned i = 0u; i < 2048u; ++i)
        sector[(mode == 1u ? 16u : 24u) + i] = (uint8_t)(seed + i);
}

int main(void)
{
    static uint8_t raw[4u * 2352u];
    static uint8_t plain[3u * 2048u];
    uint8_t output[2048];
    uint8_t form2_output[PSX_DISC_SECTOR_FORM2_DATA_SIZE];
    PsxDiscImage disc;
    PsxDiscXaSubheader xa_info;
    PsxDiscXaAudioFormat xa_format;
    const char cue1[] =
        "FILE \"game.bin\" BINARY\r\n"
        "  TRACK 01 MODE1/2352\r\n"
        "    INDEX 01 00:00:01\r\n";
    const char cue2[] =
        "FILE \"game.bin\" BINARY\n"
        "TRACK 01 MODE2/2352\n"
        "INDEX 01 00:00:00\n";
    const char cue3[] =
        "FILE \"game.iso\" BINARY\n"
        "TRACK 01 MODE1/2048\n"
        "INDEX 01 00:00:00\n";

    make_raw(raw, 1u, 0x10u);
    make_raw(raw + 2352u, 1u, 0x20u);
    make_raw(raw + 4704u, 1u, 0x30u);
    make_raw(raw + 7056u, 1u, 0x40u);
    assert(psx_disc_mount_cue(&disc, cue1, sizeof(cue1) - 1u,
                              raw, sizeof(raw)));
    assert(disc.first_sector == 1u && disc.sector_count == 3u);
    assert(psx_disc_read_sector(&disc, 0u, output));
    assert(output[0] == 0x20u && output[2047] == (uint8_t)(0x20u + 2047u));
    assert(!psx_disc_read_sector(&disc, 3u, output));
    {
        PsxDiscImage damaged = disc;
        damaged.user_data_offset = UINT16_MAX;
        assert(!psx_disc_read_sector(&damaged, 0u, output));
        damaged = disc;
        damaged.source = NULL;
        assert(!psx_disc_read_sector(&damaged, 0u, output));
        damaged = disc;
        damaged.sector_count = SIZE_MAX;
        assert(!psx_disc_read_sector(&damaged, 0u, output));
        damaged = disc;
        damaged.mode = (PsxDiscTrackMode)99;
        assert(!psx_disc_read_sector(&damaged, 0u, output));
    }
    psx_disc_unmount(&disc);
    assert(!disc.mounted);

    make_raw(raw, 2u, 0x50u);
    assert(psx_disc_mount_cue(&disc, cue2, sizeof(cue2) - 1u,
                              raw, 2352u));
    assert(psx_disc_read_sector(&disc, 0u, output));
    assert(output[0] == 0x50u);
    raw[18] = 0x20u; /* XA Form 2 is not a 2048-byte Form 1 sector. */
    for (unsigned i = 0u; i < PSX_DISC_SECTOR_FORM2_DATA_SIZE; ++i)
        raw[24u + i] = (uint8_t)(0x80u + i);
    memcpy(raw + 20u, raw + 16u, 4u);
    assert(!psx_disc_read_sector(&disc, 0u, output));
    assert(psx_disc_read_sector_form2_ex(&disc, 0u, form2_output, &xa_info));
    assert(xa_info.file == 1u && xa_info.channel == 2u);
    assert(xa_info.submode == 0x20u && xa_info.coding == 4u);
    assert(!psx_disc_xa_audio_format(&xa_info, &xa_format));
    xa_info.submode |= 0x04u;
    assert(psx_disc_xa_audio_format(&xa_info, &xa_format));
    assert(xa_format.sample_rate_hz == 18900u);
    assert(xa_format.channels == 1u && xa_format.bits_per_sample == 4u);
    xa_info.coding = 0x11u; /* Stereo, 37.8 kHz, 8-bit. */
    assert(psx_disc_xa_audio_format(&xa_info, &xa_format));
    assert(xa_format.sample_rate_hz == 37800u);
    assert(xa_format.channels == 2u && xa_format.bits_per_sample == 8u);
    xa_info.coding = 0x08u; /* Reserved/unsupported rate encoding. */
    assert(!psx_disc_xa_audio_format(&xa_info, &xa_format));
    assert(form2_output[0] == 0x80u);
    assert(form2_output[2323] == (uint8_t)(0x80u + 2323u));
    raw[20] ^= 1u; /* Duplicated XA subheader must match. */
    assert(!psx_disc_read_sector_form2(&disc, 0u, form2_output));
    raw[20] ^= 1u;
    raw[18] = 0u; /* Form 1 must not pass the Form 2 reader. */
    assert(!psx_disc_read_sector_form2(&disc, 0u, form2_output));
    assert(!psx_disc_read_sector_form2(NULL, 0u, form2_output));
    assert(!psx_disc_read_sector_form2(&disc, 1u, form2_output));

    memset(plain, 0xa5, sizeof(plain));
    assert(psx_disc_mount_cue(&disc, cue3, sizeof(cue3) - 1u,
                              plain, sizeof(plain)));
    assert(psx_disc_read_sector(&disc, 2u, output));
    assert(output[0] == 0xa5u);
    assert(!psx_disc_mount_cue(&disc, cue1, sizeof(cue1) - 1u,
                               raw, 2353u));
    {
        static const char multi_file_cue[] =
            "FILE \"a.bin\" BINARY\nFILE \"b.bin\" BINARY\n"
            "TRACK 01 MODE1/2048\nINDEX 01 00:00:00\n";
        assert(!psx_disc_mount_cue(&disc, multi_file_cue,
                                   sizeof(multi_file_cue) - 1u,
                                   plain, sizeof(plain)));
    }
    puts("disc image tests passed");
    return 0;
}
