#include "dreampsx/timers.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    PsxTimers timers;
    uint16_t half = 0u;
    uint32_t word = 0u;
    psx_timers_reset(&timers);

    /* Counter, mode and target registers occupy 4-byte slots per timer. */
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 8u, 5u));
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
                              PSX_TIMER_MODE_RESET_AT_TARGET));
    assert(psx_timers_advance(&timers, 0u, 4u));
    assert(psx_timers_read16(&timers, PSX_TIMER_BASE_ADDR, &half));
    assert(half == 4u);
    assert(psx_timers_advance(&timers, 0u, 1u));
    assert(psx_timers_read32(&timers, PSX_TIMER_BASE_ADDR + 4u, &word));
    assert((word & PSX_TIMER_MODE_REACHED_TARGET) != 0u);
    assert(psx_timers_read16(&timers, PSX_TIMER_BASE_ADDR, &half));
    assert(half == 0u);

    /* Reading MODE returns then clears event flags, not configuration. */
    assert(psx_timers_read16(&timers, PSX_TIMER_BASE_ADDR + 4u, &half));
    assert((half & PSX_TIMER_MODE_REACHED_TARGET) != 0u);
    assert(psx_timers_read16(&timers, PSX_TIMER_BASE_ADDR + 4u, &half));
    assert((half & PSX_TIMER_MODE_REACHED_TARGET) == 0u);
    assert((half & PSX_TIMER_MODE_RESET_AT_TARGET) != 0u);

    /* Timer 2 can be accessed independently and overflow is observable. */
    assert(psx_timers_write16(&timers,
        PSX_TIMER_BASE_ADDR + 2u * PSX_TIMER_STRIDE, 0xffffu));
    assert(psx_timers_advance(&timers, 2u, 1u));
    assert(psx_timers_read16(&timers,
        PSX_TIMER_BASE_ADDR + 2u * PSX_TIMER_STRIDE + 4u, &half));
    assert((half & PSX_TIMER_MODE_REACHED_OVERFLOW) != 0u);

    assert(!psx_timers_advance(&timers, 3u, 1u));
    assert(!psx_timers_read16(&timers, PSX_TIMER_BASE_ADDR + 12u, &half));
    assert(!psx_timers_read32(&timers, PSX_TIMER_BASE_ADDR + 2u, &word));
    puts("DreamPSX timer register tests: OK");
    return 0;
}
