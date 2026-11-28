#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "dreampsx/sio.h"
#include "dreampsx/kos_input_backend.h"
#include "dreampsx/irq.h"

int main(void)
{
    PsxSio sio;
    PsxIrqController irq;
    uint8_t value;
    uint16_t status;
    const uint16_t buttons = PSX_PAD_BUTTON_CROSS | PSX_PAD_BUTTON_START |
        PSX_PAD_BUTTON_LEFT | PSX_PAD_BUTTON_L2;

    psx_irq_reset(&irq);
    assert(psx_sio_init(&sio));
    psx_sio_attach_irq(&sio, &irq);
    psx_sio_set_pad_state(&sio, true, buttons);
    assert(psx_sio_write16(&sio, PSX_SIO_CTRL_ADDR,
                           PSX_SIO_CTRL_DTR | PSX_SIO_CTRL_DSRIEN));

    /* First byte asserts /ACK and latches the controller IRQ7 edge. */
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x01u));
    assert((psx_irq_read_stat(&irq) & (1u << PSX_SIO_IRQ_SOURCE)) != 0u);
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == 0xffu);
    assert(psx_sio_read16(&sio, PSX_SIO_STAT_ADDR, &status) &&
           (status & PSX_SIO_STAT_IRQ));

    /* Correct acknowledge order: I_STAT first, then SIO_CTRL.4. */
    psx_irq_write_stat(&irq, (uint16_t)~(1u << PSX_SIO_IRQ_SOURCE));
    assert(psx_sio_write8(&sio, PSX_SIO_CTRL_ADDR,
                          PSX_SIO_CTRL_DTR | PSX_SIO_CTRL_DSRIEN |
                          PSX_SIO_CTRL_ERRRST));
    assert(psx_sio_read16(&sio, PSX_SIO_STAT_ADDR, &status) &&
           !(status & PSX_SIO_STAT_IRQ));

    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x42u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == 0x41u);
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x00u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == 0x5au);
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x00u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) &&
           value == (uint8_t)~buttons);
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x00u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) &&
           value == (uint8_t)~(buttons >> 8u));
    assert(psx_sio_read8(&sio, PSX_SIO_STAT_ADDR, &value) &&
           !(value & PSX_SIO_STAT_RX_READY));
    assert(psx_sio_read16(&sio, PSX_SIO_STAT_ADDR, &status) &&
           (status & PSX_SIO_STAT_TX_READY));

    /* CTRL.6 resets SIO registers but does not unplug the controller. */
    assert(psx_sio_write16(&sio, PSX_SIO_MODE_ADDR, 0x013fu));
    assert(psx_sio_write16(&sio, PSX_SIO_BAUD_ADDR, 0x0088u));
    assert(psx_sio_write16(&sio, PSX_SIO_CTRL_ADDR, PSX_SIO_CTRL_INTRST));
    assert(psx_sio_read16(&sio, PSX_SIO_MODE_ADDR, &status) && status == 0u);
    assert(psx_sio_read16(&sio, PSX_SIO_BAUD_ADDR, &status) && status == 0u);
    assert(psx_sio_read16(&sio, PSX_SIO_STAT_ADDR, &status) && status == 5u);

    psx_sio_set_pad_state(&sio, false, 0u);
    assert(psx_sio_write16(&sio, PSX_SIO_CTRL_ADDR, PSX_SIO_CTRL_DTR));
    assert(psx_sio_write8(&sio, PSX_SIO_DATA_ADDR, 0x01u));
    assert(psx_sio_read8(&sio, PSX_SIO_DATA_ADDR, &value) && value == 0xffu);
    assert(!psx_sio_read8(&sio, 0x1f801050u, &value));
    assert(!psx_sio_write8(NULL, PSX_SIO_DATA_ADDR, 0u));
    puts("PS1 SIO digital pad and IRQ tests passed");
    return 0;
}
