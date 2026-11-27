#include <assert.h>
#include <stdint.h>
#include <stdio.h>
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

    /* Mount an in-memory ISO, ReadN LBA 0 (00:02:00), then drain its
     * 2048-byte sector through the byte data register when DRQ is enabled. */
    for (i = 0u; i < sizeof(iso); ++i) iso[i] = (uint8_t)(i & 0xffu);
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

    psx_cdrom_reset(&cdrom);
    assert(cdrom.iso_data == iso); /* reset does not eject mounted media */
    assert(!psx_cdrom_irq_output(&cdrom));
    assert(psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR, &value));
    assert((value & 0x18u) == 0x18u);
    assert(!psx_cdrom_read8(&cdrom, PSX_CDROM_BASE_ADDR + 4u, &value));
    assert(!psx_cdrom_read8(NULL, PSX_CDROM_BASE_ADDR, &value));

    puts("cdrom tests passed");
    return 0;
}
