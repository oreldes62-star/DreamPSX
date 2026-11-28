#include "dreampsx/kos_input_backend.h"

#include <dc/maple.h>
#include <dc/maple/controller.h>

bool psx_kos_input_backend_poll(PsxKosInputBackend *backend,
                                PsxKosInputState *state)
{
    maple_device_t *device;
    cont_state_t *controller;
    uint16_t buttons = 0u;

    if (!backend || !state)
        return false;

    backend->initialized = true;
    ++backend->polls;
    state->connected = false;
    state->buttons = 0u;

    device = maple_enum_type(0u, MAPLE_FUNC_CONTROLLER);
    if (!device)
        return true;

    controller = (cont_state_t *)maple_dev_status(device);
    if (!controller)
        return true;

    if (controller->buttons & CONT_START) buttons |= PSX_PAD_BUTTON_START;
    if (controller->buttons & CONT_DPAD_UP) buttons |= PSX_PAD_BUTTON_UP;
    if (controller->buttons & CONT_DPAD_RIGHT) buttons |= PSX_PAD_BUTTON_RIGHT;
    if (controller->buttons & CONT_DPAD_DOWN) buttons |= PSX_PAD_BUTTON_DOWN;
    if (controller->buttons & CONT_DPAD_LEFT) buttons |= PSX_PAD_BUTTON_LEFT;
    if (controller->buttons & CONT_A) buttons |= PSX_PAD_BUTTON_CROSS;
    if (controller->buttons & CONT_B) buttons |= PSX_PAD_BUTTON_CIRCLE;
    if (controller->buttons & CONT_X) buttons |= PSX_PAD_BUTTON_SQUARE;
    if (controller->buttons & CONT_Y) buttons |= PSX_PAD_BUTTON_TRIANGLE;
    if (controller->buttons & CONT_Z) buttons |= PSX_PAD_BUTTON_L1;
    if (controller->buttons & CONT_C) buttons |= PSX_PAD_BUTTON_R1;
    if (controller->ltrig >= 128) buttons |= PSX_PAD_BUTTON_L2;
    if (controller->rtrig >= 128) buttons |= PSX_PAD_BUTTON_R2;

    state->buttons = buttons;
    state->connected = true;
    return true;
}
