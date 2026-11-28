#ifndef DREAMPSX_SIO_H
#define DREAMPSX_SIO_H

#include <stdbool.h>
#include <stdint.h>

#define PSX_SIO_DATA_ADDR  0x1f801040u
#define PSX_SIO_STAT_ADDR  0x1f801044u
#define PSX_SIO_MODE_ADDR  0x1f801048u
#define PSX_SIO_CTRL_ADDR  0x1f80104au
#define PSX_SIO_BAUD_ADDR  0x1f80104eu

#define PSX_SIO_STAT_TX_READY (1u << 0u)
#define PSX_SIO_STAT_RX_READY (1u << 1u)
#define PSX_SIO_STAT_TX_EMPTY (1u << 2u)

typedef struct {
    uint16_t buttons;
    bool connected;
    uint8_t phase;
    uint8_t response[5];
    uint8_t response_size;
    uint8_t response_index;
    uint16_t mode;
    uint16_t control;
    uint16_t baud;
    bool initialized;
} PsxSio;

bool psx_sio_init(PsxSio *sio);
void psx_sio_set_pad_state(PsxSio *sio, bool connected, uint16_t pressed_buttons);
bool psx_sio_read8(PsxSio *sio, uint32_t address, uint8_t *value);
bool psx_sio_write8(PsxSio *sio, uint32_t address, uint8_t value);
bool psx_sio_read16(PsxSio *sio, uint32_t address, uint16_t *value);
bool psx_sio_write16(PsxSio *sio, uint32_t address, uint16_t value);

#endif
