#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dreampsx/cdrom.h"

static void select_bank(PsxCdrom *cdrom, uint8_t bank)
{
    assert(psx_cdrom_write8(cdrom, PSX_CDROM_BASE_ADDR, bank));
}

int main(void)
{
    PsxCdrom cdrom;
    uint8_t value = 0u;
    static uint8_t iso[PSX_CDROM_SECTOR_SIZE * 2u];
    unsigned i;

    assert(psx_cdrom_init(&cdrom));
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR, &value));
    assert((value & 0x18u) == 0x18u); /* parameter FIFO empty and writable */
    assert(!psx_cdrom_irq_output(&cdrom));

    /* Program INT3 enable in bank 1, register 2. */
    select_bank(&cdrom, 1u);
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x15u));
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, &value));
    assert(value == 0x15u);

    /* Setloc consumes three parameter bytes in FIFO order. */
    select_bank(&cdrom, 0u);
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x02u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x15u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x02u));
    assert(cdrom.location[0] == 0x00u);
    assert(cdrom.location[1] == 0x02u);
    assert(cdrom.location[2] == 0x15u);
    assert(cdrom.parameter_count == 0u);
    assert(psx_cdrom_irq_output(&cdrom));

    /* Read interrupt flags and the response through their proper banks. */
    select_bank(&cdrom, 1u);
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, &value));
    assert((value & 0x1fu) == 0x04u);
    select_bank(&cdrom, 0u);
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, &value));
    assert(value == 0u);

    /* Acknowledge INT3; the interrupt output must fall. */
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, 0x04u));
    assert(!psx_cdrom_irq_output(&cdrom));

    /* Getstat returns the drive status and raises INT3 again. */
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x01u));
    assert(psx_cdrom_irq_output(&cdrom));
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, &value));
    assert(value == 0u);

    /* Unknown commands report an error through INT5, not fabricated data. */
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, 0x04u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0xffu));
    assert(psx_cdrom_irq_output(&cdrom));
    select_bank(&cdrom, 1u);
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, &value));
    assert((value & 0x1fu) == 0x10u);
    select_bank(&cdrom, 0u);
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, 0x10u));

    /* Mount an in-memory ISO, ReadN LBA 0 (00:02:00), then drain its
     * 2048-byte sector through the byte data register when DRQ is enabled. */
    for (i = 0u; i < sizeof(iso); ++i)
        iso[i] = (uint8_t)(((i / PSX_CDROM_SECTOR_SIZE) * 0x40u) +
                          (i & 0xffu));
    assert(psx_cdrom_mount_iso(&cdrom, iso, sizeof(iso)));
    assert(!psx_cdrom_mount_iso(&cdrom, iso, sizeof(iso) - 1u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, 0x10u));
    select_bank(&cdrom, 0u);
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x02u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x02u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x06u));
    assert(cdrom.data_count == PSX_CDROM_SECTOR_SIZE);
    select_bank(&cdrom, 2u);
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x80u));
    select_bank(&cdrom, 0u);
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, &value));
    assert(value == 0u);
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, &value));
    assert(value == 1u);
    for (i = 2u; i < PSX_CDROM_SECTOR_SIZE; ++i)
        assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, &value));
    assert(value == (uint8_t)((PSX_CDROM_SECTOR_SIZE - 1u) & 0xffu));
    assert(cdrom.data_count == 0u);
    assert((cdrom.drive_status & 0x20u) == 0u);
    assert(psx_cdrom_irq_output(&cdrom)); /* INT1 remains latched to ack. */

    /* ReadN remains active: acknowledge prior reasons, then request the next
     * timed sector. The second sector has a distinct byte pattern. */
    select_bank(&cdrom, 0u);
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, 0x05u));
    assert(psx_cdrom_advance_sector(&cdrom));
    assert(cdrom.data_count == PSX_CDROM_SECTOR_SIZE);
    assert(psx_cdrom_irq_output(&cdrom));
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, &value));
    assert(value == 0x40u);
    for (i = 1u; i < PSX_CDROM_SECTOR_SIZE; ++i)
        assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, &value));
    assert(value == (uint8_t)(0x40u + 0xffu));
    assert(cdrom.data_count == 0u);
    assert(!psx_cdrom_advance_sector(&cdrom)); /* End of mounted image. */
    assert(!cdrom.read_active);
    select_bank(&cdrom, 1u);
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, &value));
    assert((value & 0x10u) != 0u); /* INT5 reports end-of-image error. */

    /* A raw MODE1/2352 CUE track now feeds the same ReadN FIFO. */
    {
        static uint8_t raw_track[2352u];
        static const char cue[] =
            "FILE \"disc.bin\" BINARY\n"
            "TRACK 01 MODE1/2352\n"
            "INDEX 01 00:00:00\n";
        uint32_t word = 0u;
        memset(raw_track, 0, sizeof(raw_track));
        for (i = 1u; i <= 10u; ++i) raw_track[i] = 0xffu;
        raw_track[15] = 1u;
        raw_track[16] = 0x11u;
        raw_track[17] = 0x22u;
        raw_track[18] = 0x33u;
        raw_track[19] = 0x44u;
        assert(psx_cdrom_mount_cue(&cdrom, cue, sizeof(cue) - 1u,
                                   raw_track, sizeof(raw_track)));
        assert(cdrom.disc.mounted && cdrom.iso_data == NULL);
        select_bank(&cdrom, 0u);
        assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
        assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x02u));
        assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
        assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x02u));
        assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x06u));
        select_bank(&cdrom, 2u);
        assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x80u));
        assert(psx_cdrom_read_data32(&cdrom, &word));
        assert(word == 0x44332211u);
        assert(cdrom.data_count == PSX_CDROM_SECTOR_SIZE - 4u);
        psx_cdrom_unmount(&cdrom);
        assert(!cdrom.disc.mounted && cdrom.iso_data == NULL);
    }

    psx_cdrom_reset(&cdrom);
    assert(cdrom.iso_data == iso); /* reset does not eject mounted media */
    assert(!psx_cdrom_irq_output(&cdrom));
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR, &value));
    assert((value & 0x18u) == 0x18u);

    /* Setmode selects the drive transfer speed; reset defaults to normal. */
    assert(psx_cdrom_sector_period_cycles(&cdrom) ==
           PSX_CDROM_NORMAL_SPEED_SECTOR_CYCLES);
    select_bank(&cdrom, 0u);
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x80u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x0eu));
    assert(cdrom.mode == 0x80u);
    assert(psx_cdrom_sector_period_cycles(&cdrom) ==
           PSX_CDROM_DOUBLE_SPEED_SECTOR_CYCLES);

    /* Pause acknowledges immediately but completes asynchronously. */
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, 0x1fu));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x09u));
    assert(cdrom.pending_command == 0x09u);
    assert(cdrom.command_cycles_remaining ==
           PSX_CDROM_PAUSE_DOUBLE_CYCLES);
    psx_cdrom_advance_cycles(&cdrom, PSX_CDROM_PAUSE_DOUBLE_CYCLES - 1u);
    assert(cdrom.pending_command == 0x09u);
    assert((cdrom.interrupt_flags & (1u << 1u)) == 0u);
    psx_cdrom_advance_cycles(&cdrom, 1u);
    assert(cdrom.pending_command == 0u);
    assert((cdrom.interrupt_flags & (1u << 1u)) != 0u);

    /* Stop has its own measured completion interval and clears buffered data. */
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, 0x1fu));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x08u));
    assert(cdrom.pending_command == 0x08u);
    assert(cdrom.command_cycles_remaining ==
           PSX_CDROM_STOP_DOUBLE_CYCLES);
    psx_cdrom_advance_cycles(&cdrom, PSX_CDROM_STOP_DOUBLE_CYCLES);
    assert(cdrom.pending_command == 0u);
    assert((cdrom.interrupt_flags & (1u << 1u)) != 0u);
    assert((cdrom.drive_status & 0x02u) == 0u);

    /* Returning to normal speed doubles the sector interval. */
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 3u, 0x1fu));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 2u, 0x00u));
    assert(psx_cdrom_write8(&cdrom, PSX_CDROM_BASE_ADDR + 1u, 0x0eu));
    assert(psx_cdrom_sector_period_cycles(&cdrom) ==
           PSX_CDROM_NORMAL_SPEED_SECTOR_CYCLES);
    assert(!psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 4u, &value));
    assert(!psx_cdrom_read8(NULL, PSX_CDROM_BASE_ADDR, &value));

    puts("cdrom tests passed");
    return 0;
}
