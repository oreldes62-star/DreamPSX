#ifndef DREAMPSX_KOS_INPUT_BACKEND_H
#define DREAMPSX_KOS_INPUT_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#define PSX_PAD_BUTTON_SELECT   (1u << 0u)
#define PSX_PAD_BUTTON_L3       (1u << 1u)
#define PSX_PAD_BUTTON_R3       (1u << 2u)
#define PSX_PAD_BUTTON_START    (1u << 3u)
#define PSX_PAD_BUTTON_UP       (1u << 4u)
#define PSX_PAD_BUTTON_RIGHT    (1u << 5u)
#define PSX_PAD_BUTTON_DOWN     (1u << 6u)
#define PSX_PAD_BUTTON_LEFT     (1u << 7u)
#define PSX_PAD_BUTTON_L2       (1u << 8u)
#define PSX_PAD_BUTTON_R2       (1u << 9u)
#define PSX_PAD_BUTTON_L1       (1u << 10u)
#define PSX_PAD_BUTTON_R1       (1u << 11u)
#define PSX_PAD_BUTTON_TRIANGLE (1u << 12u)
#define PSX_PAD_BUTTON_CIRCLE   (1u << 13u)
#define PSX_PAD_BUTTON_CROSS    (1u << 14u)
#define PSX_PAD_BUTTON_SQUARE   (1u << 15u)

typedef struct {
    uint16_t buttons;
    bool connected;
} PsxKosInputState;

typedef struct {
    bool initialized;
    uint32_t polls;
} PsxKosInputBackend;

/* Polls the first connected Dreamcast controller and maps it to PS1 buttons.
 * The PS1 button word uses one bits for pressed buttons; the SIO layer will
 * invert it when producing the active-low digital-pad response.
 */
bool psx_kos_input_backend_poll(PsxKosInputBackend *backend,
                                PsxKosInputState *state);

#endif
