#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "dreampsx/sio.h"
#include "dreampsx/kos_input_backend.h"

int main(void)
{
    PsxSio sio;
    uint8_t value;
    uint16_t status;
    const uint16_t buttons = PSX_PAD_BUTTON_CROSS | PSX_PAD_BUTTON_START |
        PSX_PAD_BUTTON_LEFT | PSX_PAD_BUTTON_L2;

    assert(psx_sio_init(&sio));
    psx_sio_set_pad_state(&sio, true, buttons);
    assert(psx_sio_write16(&sio, PSX_SIO_CTRL_ADDR, 0x0002u));

    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x01u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == 0xffu);
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x42u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == 0x41u);
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x00u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == 0x5au);
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x00u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == (uint8_t)~buttons);
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x00u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == (uint8_t)~(buttons >> 8u));
    assert(psx_sio_read8(&sio, PSX_SIO_STAT_ADDR, &value) && !(value & PSX_SIO_STAT_RX_READY));
    assert(psx_sio_read16(&sio, PSX_SIO_STAT_ADDR, &status) && (status & PSX_SIO_STAT_TX_READY));

    psx_sio_set_pad_state(&sio, false, 0u);
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x01u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == 0xffu);
    assert(!psx_sio_read8(&sio, 0x1f801050u, &value));
    assert(!psx_sio_write8(NULL, PSX_SIO_DATA_ADDR, 0u));
    puts("PS1 SIO digital pad protocol tests passed");
    return 0;
}
