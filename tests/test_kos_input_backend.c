#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "dc/maple.h"
#include "dc/maple/controller.h"
#include "dreampsx/kos_input_backend.h"

static maple_device_t test_device;
static cont_state_t test_controller;
static bool device_present = true;
static bool status_present = true;

maple_device_t *maple_enum_type(size_t n, uint32_t func)
{
    assert(n == 0u);
    assert(func == MAPLE_FUNC_CONTROLLER);
    return device_present ? &test_device : NULL;
}

void *maple_dev_status(maple_device_t *device)
{
    assert(device == &test_device);
    return status_present ? &test_controller : NULL;
}

int main(void)
{
    PsxKosInputBackend backend = {0};
    PsxKosInputState state;

    test_controller.buttons = CONT_START | CONT_DPAD_UP | CONT_DPAD_RIGHT |
        CONT_A | CONT_B | CONT_X | CONT_Y | CONT_Z | CONT_C;
    test_controller.ltrig = 200;
    test_controller.rtrig = 127;

    assert(psx_kos_input_backend_poll(&backend, &state));
    assert(backend.initialized && backend.polls == 1u);
    assert(state.connected);
    assert(state.buttons == (PSX_PAD_BUTTON_START | PSX_PAD_BUTTON_UP |
        PSX_PAD_BUTTON_RIGHT | PSX_PAD_BUTTON_CROSS | PSX_PAD_BUTTON_CIRCLE |
        PSX_PAD_BUTTON_SQUARE | PSX_PAD_BUTTON_TRIANGLE |
        PSX_PAD_BUTTON_L1 | PSX_PAD_BUTTON_R1 | PSX_PAD_BUTTON_L2));
    assert((state.buttons & PSX_PAD_BUTTON_R2) == 0u);

    device_present = false;
    assert(psx_kos_input_backend_poll(&backend, &state));
    assert(!state.connected && state.buttons == 0u);

    device_present = true;
    status_present = false;
    assert(psx_kos_input_backend_poll(&backend, &state));
    assert(!state.connected && state.buttons == 0u);

    assert(!psx_kos_input_backend_poll(NULL, &state));
    assert(!psx_kos_input_backend_poll(&backend, NULL));
    puts("KallistiOS input backend tests passed");
    return 0;
}
