#include "dreampsx/sio.h"
#include <string.h>

static void pulse_irq(PsxSio *sio)
{
    if (!sio) return;
    sio->irq_pending = true;
    if (sio->irq) {
        psx_irq_set_line(sio->irq, PSX_SIO_IRQ_SOURCE, true);
        psx_irq_set_line(sio->irq, PSX_SIO_IRQ_SOURCE, false);
    }
}

static void queue_byte(PsxSio *sio, uint8_t value)
{
    sio->response[0] = value;
    sio->response_size = 1u;
    sio->response_index = 0u;
}

bool psx_sio_init(PsxSio *sio)
{
    if (!sio) return false;
    memset(sio, 0, sizeof(*sio));
    sio->initialized = true;
    return true;
}

void psx_sio_reset(PsxSio *sio)
{
    PsxIrqController *irq;
    uint16_t buttons;
    bool connected;
    if (!sio || !sio->initialized) return;
    irq = sio->irq;
    buttons = sio->buttons;
    connected = sio->connected;
    memset(sio, 0, sizeof(*sio));
    sio->irq = irq;
    sio->buttons = buttons;
    sio->connected = connected;
    sio->initialized = true;
}

void psx_sio_attach_irq(PsxSio *sio, PsxIrqController *irq)
{
    if (sio && sio->initialized) sio->irq = irq;
}

void psx_sio_set_pad_state(PsxSio *sio, bool connected, uint16_t pressed_buttons)
{
    if (!sio || !sio->initialized) return;
    sio->connected = connected;
    sio->buttons = connected ? pressed_buttons : 0u;
}

static void sio_data_write(PsxSio *sio, uint8_t value)
{
    uint8_t response = 0xffu;
    bool acknowledged = false;

    /* SIO0 /CS is DTR (CTRL.1). RXEN (CTRL.2) allows one forced byte. */
    if (!sio->connected ||
        (!(sio->control & PSX_SIO_CTRL_DTR) &&
         !(sio->control & PSX_SIO_CTRL_RXEN))) {
        sio->phase = 0u;
        queue_byte(sio, response);
        return;
    }

    switch (sio->phase) {
    case 0u:
        if (value == 0x01u) {
            sio->phase = 1u;
            acknowledged = true;
        } else {
            sio->phase = 0u;
        }
        break;
    case 1u:
        response = 0x41u; /* Digital pad ID, two button bytes follow. */
        sio->phase = value == 0x42u ? 2u : 0u;
        acknowledged = true;
        break;
    case 2u:
        response = 0x5au;
        sio->phase = 3u;
        acknowledged = true;
        break;
    case 3u:
        response = (uint8_t)~sio->buttons;
        sio->phase = 4u;
        acknowledged = true;
        break;
    case 4u:
        response = (uint8_t)~(sio->buttons >> 8u);
        sio->phase = 0u;
        /* The final data byte has no further /ACK pulse on a standard pad. */
        break;
    default:
        sio->phase = 0u;
        break;
    }

    queue_byte(sio, response);

    if (sio->control & PSX_SIO_CTRL_TXIEN)
        pulse_irq(sio);
    if ((acknowledged && (sio->control & PSX_SIO_CTRL_DSRIEN)) ||
        ((sio->control & PSX_SIO_CTRL_RXIEN) &&
         sio->response_size >= (1u << ((sio->control >> 8u) & 3u))))
        pulse_irq(sio);
}

bool psx_sio_read8(PsxSio *sio, uint32_t address, uint8_t *value)
{
    if (!sio || !sio->initialized || !value) return false;
    if (address == PSX_SIO_DATA_ADDR) {
        if (sio->response_index >= sio->response_size) {
            *value = 0xffu;
            return true;
        }
        *value = sio->response[sio->response_index++];
        if (sio->response_index >= sio->response_size) {
            sio->response_size = 0u;
            sio->response_index = 0u;
        }
        return true;
    }
    if (address == PSX_SIO_STAT_ADDR) {
        uint16_t status = PSX_SIO_STAT_TX_READY | PSX_SIO_STAT_TX_EMPTY;
        if (sio->response_size) status |= PSX_SIO_STAT_RX_READY;
        if (sio->connected && sio->response_size) status |= PSX_SIO_STAT_DSR;
        if (sio->irq_pending) status |= PSX_SIO_STAT_IRQ;
        *value = (uint8_t)status;
        return true;
    }
    if (address == PSX_SIO_MODE_ADDR) { *value = (uint8_t)sio->mode; return true; }
    if (address == PSX_SIO_MODE_ADDR + 1u) { *value = (uint8_t)(sio->mode >> 8u); return true; }
    if (address == PSX_SIO_CTRL_ADDR) { *value = (uint8_t)sio->control; return true; }
    if (address == PSX_SIO_CTRL_ADDR + 1u) { *value = (uint8_t)(sio->control >> 8u); return true; }
    if (address == PSX_SIO_BAUD_ADDR) { *value = (uint8_t)sio->baud; return true; }
    if (address == PSX_SIO_BAUD_ADDR + 1u) { *value = (uint8_t)(sio->baud >> 8u); return true; }
    return false;
}

bool psx_sio_write8(PsxSio *sio, uint32_t address, uint8_t value)
{
    if (!sio || !sio->initialized) return false;
    if (address == PSX_SIO_DATA_ADDR) { sio_data_write(sio, value); return true; }
    if (address == PSX_SIO_MODE_ADDR) { sio->mode = (uint16_t)((sio->mode & 0xff00u) | value); return true; }
    if (address == PSX_SIO_MODE_ADDR + 1u) { sio->mode = (uint16_t)((sio->mode & 0x00ffu) | ((uint16_t)value << 8u)); return true; }
    if (address == PSX_SIO_CTRL_ADDR) {
        uint16_t control = (uint16_t)((sio->control & 0xff00u) | value);
        if (value & PSX_SIO_CTRL_ERRRST) {
            sio->irq_pending = false;
            if (sio->irq) psx_irq_set_line(sio->irq, PSX_SIO_IRQ_SOURCE, false);
        }
        if (value & PSX_SIO_CTRL_INTRST) {
            psx_sio_reset(sio);
            return true;
        }
        sio->control = (uint16_t)(control & (uint16_t)~PSX_SIO_CTRL_ERRRST);
        return true;
    }
    if (address == PSX_SIO_CTRL_ADDR + 1u) {
        sio->control = (uint16_t)((sio->control & 0x00ffu) | ((uint16_t)value << 8u));
        return true;
    }
    if (address == PSX_SIO_BAUD_ADDR) { sio->baud = (uint16_t)((sio->baud & 0xff00u) | value); return true; }
    if (address == PSX_SIO_BAUD_ADDR + 1u) { sio->baud = (uint16_t)((sio->baud & 0x00ffu) | ((uint16_t)value << 8u)); return true; }
    return false;
}

bool psx_sio_read16(PsxSio *sio, uint32_t address, uint16_t *value)
{
    uint8_t low;
    if (!sio || !sio->initialized || !value) return false;
    if (address == PSX_SIO_DATA_ADDR) {
        if (!psx_sio_read8(sio, address, &low)) return false;
        *value = (uint16_t)(low | 0xff00u);
        return true;
    }
    if (address == PSX_SIO_STAT_ADDR) {
        uint16_t status = PSX_SIO_STAT_TX_READY | PSX_SIO_STAT_TX_EMPTY;
        if (sio->response_size) status |= PSX_SIO_STAT_RX_READY;
        if (sio->connected && sio->response_size) status |= PSX_SIO_STAT_DSR;
        if (sio->irq_pending) status |= PSX_SIO_STAT_IRQ;
        *value = status;
        return true;
    }
    if (address == PSX_SIO_MODE_ADDR) { *value = sio->mode; return true; }
    if (address == PSX_SIO_CTRL_ADDR) { *value = sio->control; return true; }
    if (address == PSX_SIO_BAUD_ADDR) { *value = sio->baud; return true; }
    return false;
}

bool psx_sio_write16(PsxSio *sio, uint32_t address, uint16_t value)
{
    if (!sio || !sio->initialized) return false;
    if (address == PSX_SIO_DATA_ADDR) return psx_sio_write8(sio, address, (uint8_t)value);
    if (address == PSX_SIO_MODE_ADDR) { sio->mode = value; return true; }
    if (address == PSX_SIO_CTRL_ADDR) {
        if (value & PSX_SIO_CTRL_INTRST) { psx_sio_reset(sio); return true; }
        if (value & PSX_SIO_CTRL_ERRRST) {
            sio->irq_pending = false;
            if (sio->irq) psx_irq_set_line(sio->irq, PSX_SIO_IRQ_SOURCE, false);
        }
        sio->control = (uint16_t)(value & (uint16_t)~PSX_SIO_CTRL_ERRRST);
        return true;
    }
    if (address == PSX_SIO_BAUD_ADDR) { sio->baud = value; return true; }
    return false;
}
