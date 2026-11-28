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
    /* Reading MODE returns then clears event flags, not configuration. */
    assert(psx_timers_read16(&timers, PSX_TIMER_BASE_ADDR + 4u, &half));
    assert((half & PSX_TIMER_MODE_REACHED_TARGET) != 0u);
    assert(psx_timers_read16(&timers, PSX_TIMER_BASE_ADDR, &half));
    assert(half == 0u);
    assert(psx_timers_read16(&timers, PSX_TIMER_BASE_ADDR + 4u, &half));
    assert((half & PSX_TIMER_MODE_REACHED_TARGET) == 0u);
    assert((half & PSX_TIMER_MODE_RESET_AT_TARGET) != 0u);
    assert(psx_timers_read32(&timers, PSX_TIMER_BASE_ADDR + 4u, &word));
    assert((word & PSX_TIMER_MODE_REACHED_TARGET) == 0u);

    /* Timer 2 can be accessed independently and overflow is observable. */
    assert(psx_timers_write16(&timers,
        PSX_TIMER_BASE_ADDR + 2u * PSX_TIMER_STRIDE, 0xffffu));
    assert(psx_timers_advance(&timers, 2u, 1u));
    assert(psx_timers_read16(&timers,
        PSX_TIMER_BASE_ADDR + 2u * PSX_TIMER_STRIDE + 4u, &half));
    assert((half & PSX_TIMER_MODE_REACHED_OVERFLOW) != 0u);

    /* Clock selection: Timer 0 can be driven by dot clock; Timer 1 by HBlank;
     * Timer 2 by system/8. IRQ request is emitted once unless repeat is set.
     */
    psx_timers_reset(&timers);
    uint8_t irq = 0xffu;
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 8u, 2u));
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(1u << PSX_TIMER_MODE_CLOCK_SHIFT) |
        PSX_TIMER_MODE_IRQ_TARGET));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    4u, &irq));
    assert(irq == 0u);
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_DOT,
                                    2u, &irq));
    assert(irq == 1u);
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_DOT,
                                    2u, &irq));
    assert(irq == 0u); /* one-shot IRQ */
    /* Sync mode 0 pauses the system clock during blanking. */
    psx_timers_reset(&timers);
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_SYNC_ENABLE |
                   (0u << PSX_TIMER_MODE_SYNC_SHIFT))));
    assert(psx_timers_set_sync_signal(&timers, 0u, true));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    5u, &irq));
    assert(timers.timer[0].counter == 0u);
    assert(psx_timers_set_sync_signal(&timers, 0u, false));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    3u, &irq));
    assert(timers.timer[0].counter == 3u);

    /* Mode 1 resets on the blanking edge, then continues counting. */
    psx_timers_reset(&timers);
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_SYNC_ENABLE |
                   (1u << PSX_TIMER_MODE_SYNC_SHIFT))));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    4u, &irq));
    assert(timers.timer[0].counter == 4u);
    assert(psx_timers_set_sync_signal(&timers, 0u, true));
    assert(timers.timer[0].counter == 0u);
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    2u, &irq));
    assert(timers.timer[0].counter == 2u);

    /* Mode 2 resets at blanking and counts only while blanking is active. */
    psx_timers_reset(&timers);
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_SYNC_ENABLE |
                   (2u << PSX_TIMER_MODE_SYNC_SHIFT))));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    4u, &irq));
    assert(timers.timer[0].counter == 0u);
    assert(psx_timers_set_sync_signal(&timers, 0u, true));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    3u, &irq));
    assert(timers.timer[0].counter == 3u);
    assert(psx_timers_set_sync_signal(&timers, 0u, false));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    2u, &irq));
    assert(timers.timer[0].counter == 3u);

    /* Reprogramming MODE clears stale blanking state and restarts sync.
     * This prevents a mode change during blanking from leaving a timer gated
     * by an edge that belonged to the previous configuration.
     */
    psx_timers_reset(&timers);
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_SYNC_ENABLE |
                   (2u << PSX_TIMER_MODE_SYNC_SHIFT))));
    assert(psx_timers_set_sync_signal(&timers, 0u, true));
    assert(timers.timer[0].sync_signal_active);
    assert(timers.timer[0].sync_started);
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_SYNC_ENABLE |
                   (0u << PSX_TIMER_MODE_SYNC_SHIFT))));
    assert(!timers.timer[0].sync_signal_active);
    assert(!timers.timer[0].sync_started);
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    3u, &irq));
    assert(timers.timer[0].counter == 3u);

    /* Mode 3 waits for the first blanking edge, then runs freely. */
    psx_timers_reset(&timers);
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(PSX_TIMER_MODE_SYNC_ENABLE |
                   (3u << PSX_TIMER_MODE_SYNC_SHIFT))));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    4u, &irq));
    assert(timers.timer[0].counter == 0u);
    assert(psx_timers_set_sync_signal(&timers, 0u, true));
    assert(psx_timers_set_sync_signal(&timers, 0u, false));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    2u, &irq));
    assert(timers.timer[0].counter == 2u);

    /* Clock-source bit 1 mirrors bit 0 for each timer: source 2
     * is the system clock and source 3 selects the external/divided clock.
     */
    psx_timers_reset(&timers);
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(2u << PSX_TIMER_MODE_CLOCK_SHIFT)));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    2u, &irq));
    assert(timers.timer[0].counter == 2u);
    assert(psx_timers_write16(&timers, PSX_TIMER_BASE_ADDR + 4u,
        (uint16_t)(3u << PSX_TIMER_MODE_CLOCK_SHIFT)));
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_SYSTEM,
                                    2u, &irq));
    assert(timers.timer[0].counter == 2u);
    assert(psx_timers_advance_clock(&timers, 0u, PSX_TIMER_CLOCK_DOT,
                                    3u, &irq));
    assert(timers.timer[0].counter == 5u);

    psx_timers_reset(&timers);
    assert(psx_timers_write16(&timers,
        PSX_TIMER_BASE_ADDR + PSX_TIMER_STRIDE + 4u,
        (uint16_t)(3u << PSX_TIMER_MODE_CLOCK_SHIFT)));
    assert(psx_timers_advance_clock(&timers, 1u, PSX_TIMER_CLOCK_SYSTEM,
                                    2u, &irq));
    assert(timers.timer[1].counter == 0u);
    assert(psx_timers_advance_clock(&timers, 1u, PSX_TIMER_CLOCK_HBLANK,
                                    3u, &irq));
    assert(timers.timer[1].counter == 3u);

    psx_timers_reset(&timers);
    assert(psx_timers_write16(&timers,
        PSX_TIMER_BASE_ADDR + 2u * PSX_TIMER_STRIDE + 4u,
        (uint16_t)(3u << PSX_TIMER_MODE_CLOCK_SHIFT)));
    assert(psx_timers_advance_clock(&timers, 2u, PSX_TIMER_CLOCK_SYSTEM,
                                    2u, &irq));
    assert(timers.timer[2].counter == 0u);
    assert(psx_timers_advance_clock(&timers, 2u,
                                    PSX_TIMER_CLOCK_SYSTEM_DIV8, 3u, &irq));
    assert(timers.timer[2].counter == 3u);

    assert(!psx_timers_advance_clock(&timers, 3u, PSX_TIMER_CLOCK_SYSTEM,
                                     1u, &irq));

    assert(!psx_timers_advance(&timers, 3u, 1u));
    assert(!psx_timers_read16(&timers, PSX_TIMER_BASE_ADDR + 12u, &half));
    assert(!psx_timers_read32(&timers, PSX_TIMER_BASE_ADDR + 2u, &word));
    puts("DreamPSX timer register tests: OK");
    return 0;
}
