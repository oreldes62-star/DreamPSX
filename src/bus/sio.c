#include "dreampsx/sio.h"

#include <string.h>

static void queue_response(PsxSio *sio, const uint8_t *bytes, uint8_t count)
{
    memcpy(sio->response, bytes, count);
    sio->response_size = count;
    sio->response_index = 0u;
}

bool psx_sio_init(PsxSio *sio)
{
    if (!sio) return false;
    memset(sio, 0, sizeof(*sio));
    sio->initialized = true;
    return true;
}

void psx_sio_set_pad_state(PsxSio *sio, bool connected, uint16_t pressed_buttons)
{
    if (!sio || !sio->initialized) return;
    sio->connected = connected;
    sio->buttons = connected ? pressed_buttons : 0u;
}

static uint8_t sio_data_read(PsxSio *sio)
{
    uint8_t value;
    if (sio->response_index >= sio->response_size) return 0xffu;
    value = sio->response[sio->response_index++];
    if (sio->response_index >= sio->response_size) {
        sio->response_size = 0u;
        sio->response_index = 0u;
    }
    return value;
}

static void sio_data_write(PsxSio *sio, uint8_t value)
{
    static const uint8_t ack[] = {0xffu};
    static const uint8_t id[] = {0x41u};
    static const uint8_t poll_header[] = {0x5au};
    uint8_t button_byte;

    if (!sio->connected || !(sio->control & 0x0002u)) {
        sio->phase = 0u;
        queue_response(sio, ack, 1u);
        return;
    }

    switch (sio->phase) {
    case 0u:
        sio->phase = value == 0x01u ? 1u : 0u;
        queue_response(sio, ack, 1u);
        break;
    case 1u:
        sio->phase = value == 0x42u ? 2u : 0u;
        queue_response(sio, id, 1u);
        break;
    case 2u:
        sio->phase = 3u;
        queue_response(sio, poll_header, 1u);
        break;
    case 3u:
        button_byte = (uint8_t)~sio->buttons;
        sio->phase = 4u;
        queue_response(sio, &button_byte, 1u);
        break;
    case 4u:
        button_byte = (uint8_t)~(sio->buttons >> 8u);
        sio->phase = 0u;
        queue_response(sio, &button_byte, 1u);
        break;
    default:
        sio->phase = 0u;
        queue_response(sio, ack, 1u);
        break;
    }
}

bool psx_sio_read8(PsxSio *sio, uint32_t address, uint8_t *value)
{
    if (!sio || !sio->initialized || !value) return false;
    if (address == PSX_SIO_DATA_ADDR) {
        *value = sio_data_read(sio);
        return true;
    }
    if (address == PSX_SIO_STAT_ADDR) {
        *value = (uint8_t)(PSX_SIO_STAT_TX_READY | PSX_SIO_STAT_TX_EMPTY |
            (sio->response_size ? PSX_SIO_STAT_RX_READY : 0u));
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
        sio->control = (uint16_t)((sio->control & 0xff00u) | value);
        if (value & 0x10u) { sio->phase = 0u; sio->response_size = 0u; sio->response_index = 0u; }
        return true;
    }
    if (address == PSX_SIO_CTRL_ADDR + 1u) { sio->control = (uint16_t)((sio->control & 0x00ffu) | ((uint16_t)value << 8u)); return true; }
    if (address == PSX_SIO_BAUD_ADDR) { sio->baud = (uint16_t)((sio->baud & 0xff00u) | value); return true; }
    if (address == PSX_SIO_BAUD_ADDR + 1u) { sio->baud = (uint16_t)((sio->baud & 0x00ffu) | ((uint16_t)value << 8u)); return true; }
    return false;
}

bool psx_sio_read16(PsxSio *sio, uint32_t address, uint16_t *value)
{
    uint8_t low, high;
    if (!value) return false;
    if (address == PSX_SIO_DATA_ADDR) {
        if (!psx_sio_read8(sio, address, &low)) return false;
        if (!psx_sio_read8(sio, address, &high)) return false;
        *value = (uint16_t)(low | ((uint16_t)high << 8u));
        return true;
    }
    if (address == PSX_SIO_STAT_ADDR) {
        *value = (uint16_t)(PSX_SIO_STAT_TX_READY | PSX_SIO_STAT_TX_EMPTY |
            (sio && sio->response_size ? PSX_SIO_STAT_RX_READY : 0u));
        return sio && sio->initialized;
    }
    if (address == PSX_SIO_MODE_ADDR) { *value = sio->mode; return sio && sio->initialized; }
    if (address == PSX_SIO_CTRL_ADDR) { *value = sio->control; return sio && sio->initialized; }
    if (address == PSX_SIO_BAUD_ADDR) { *value = sio->baud; return sio && sio->initialized; }
    return false;
}

bool psx_sio_write16(PsxSio *sio, uint32_t address, uint16_t value)
{
    if (address == PSX_SIO_DATA_ADDR) return psx_sio_write8(sio, address, (uint8_t)value);
    if (address == PSX_SIO_MODE_ADDR) { if (!sio || !sio->initialized) return false; sio->mode = value; return true; }
    if (address == PSX_SIO_CTRL_ADDR) { if (!sio || !sio->initialized) return false; sio->control = value; if (value & 0x10u) { sio->phase = 0u; sio->response_size = 0u; sio->response_index = 0u; } return true; }
    if (address == PSX_SIO_BAUD_ADDR) { if (!sio || !sio->initialized) return false; sio->baud = value; return true; }
    return false;
}
