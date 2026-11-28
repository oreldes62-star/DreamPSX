#include "dreampsx/timers.h"

static bool decode_address(uint32_t address, unsigned *timer,
                           unsigned *reg)
{
    if (address < PSX_TIMER_BASE_ADDR) return false;
    const uint32_t offset = address - PSX_TIMER_BASE_ADDR;
    const unsigned index = (unsigned)(offset / PSX_TIMER_STRIDE);
    const unsigned slot = (unsigned)(offset % PSX_TIMER_STRIDE);
    if (index >= PSX_TIMER_COUNT || slot > 8u || (slot & 3u) != 0u)
        return false;
    *timer = index;
    *reg = slot / 4u;
    return true;
}

void psx_timers_reset(PsxTimers *timers)
{
    if (!timers) return;
    for (unsigned i = 0u; i < PSX_TIMER_COUNT; ++i) {
        timers->timer[i].counter = 0u;
        timers->timer[i].mode = 0u;
        timers->timer[i].target = 0u;
        timers->timer[i].irq_fired = false;
        timers->timer[i].irq_toggle_state = false;
        timers->timer[i].sync_signal_active = false;
        timers->timer[i].sync_started = false;
    }
}

bool psx_timers_read16(PsxTimers *timers, uint32_t address,
                       uint16_t *value)
{
    unsigned index, reg;
    if (!timers || !value || !decode_address(address, &index, &reg))
        return false;
    PsxTimer *timer = &timers->timer[index];
    if (reg == 0u) {
        *value = timer->counter;
    } else if (reg == 1u) {
        *value = timer->mode;
        timer->mode &= (uint16_t)~(PSX_TIMER_MODE_REACHED_TARGET |
                                  PSX_TIMER_MODE_REACHED_OVERFLOW);
    } else {
        *value = timer->target;
    }
    return true;
}

bool psx_timers_read32(PsxTimers *timers, uint32_t address,
                       uint32_t *value)
{
    uint16_t half;
    if (!value || (address & 3u) != 0u ||
        !psx_timers_read16(timers, address, &half))
        return false;
    *value = half;
    return true;
}

bool psx_timers_write16(PsxTimers *timers, uint32_t address,
                        uint16_t value)
{
    unsigned index, reg;
    if (!timers || !decode_address(address, &index, &reg)) return false;
    PsxTimer *timer = &timers->timer[index];
    if (reg == 0u) {
        timer->counter = value;
    } else if (reg == 1u) {
        /* Mode writes reset the counter and clear read-only event flags. */
        timer->counter = 0u;
        timer->mode = (uint16_t)(value & 0x03ffu);
        timer->irq_fired = false;
        timer->irq_toggle_state = false;
        /* The external blanking signal is independent of MODE writes. */
        timer->sync_started = false;
    } else {
        timer->target = value;
    }
    return true;
}

bool psx_timers_write32(PsxTimers *timers, uint32_t address,
                        uint32_t value)
{
    if ((address & 3u) != 0u) return false;
    return psx_timers_write16(timers, address, (uint16_t)value);
}

bool psx_timers_advance(PsxTimers *timers, unsigned index,
                        uint32_t ticks)
{
    if (!timers || index >= PSX_TIMER_COUNT) return false;
    PsxTimer *timer = &timers->timer[index];
    while (ticks-- != 0u) {
        timer->counter = (uint16_t)(timer->counter + 1u);
        if (timer->counter == timer->target) {
            timer->mode |= PSX_TIMER_MODE_REACHED_TARGET;
            if ((timer->mode & PSX_TIMER_MODE_RESET_AT_TARGET) != 0u)
                timer->counter = 0u;
        }
        if (timer->counter == 0u &&
            (timer->mode & PSX_TIMER_MODE_RESET_AT_TARGET) == 0u)
            timer->mode |= PSX_TIMER_MODE_REACHED_OVERFLOW;
    }
    return true;
}

static PsxTimerClock selected_clock(unsigned index, uint16_t mode)
{
    const unsigned source = (mode >> PSX_TIMER_MODE_CLOCK_SHIFT) & 3u;
    if (index == 0u)
        return (source & 1u) != 0u ?
               PSX_TIMER_CLOCK_DOT : PSX_TIMER_CLOCK_SYSTEM;
    if (index == 1u)
        return (source & 1u) != 0u ?
               PSX_TIMER_CLOCK_HBLANK : PSX_TIMER_CLOCK_SYSTEM;
    return (source & 1u) != 0u ?
           PSX_TIMER_CLOCK_SYSTEM_DIV8 : PSX_TIMER_CLOCK_SYSTEM;
}

bool psx_timers_set_sync_signal(PsxTimers *timers, unsigned index,
                                bool active)
{
    if (!timers || index >= 2u) return false;
    PsxTimer *timer = &timers->timer[index];
    const bool rising = active && !timer->sync_signal_active;
    timer->sync_signal_active = active;
    if ((timer->mode & PSX_TIMER_MODE_SYNC_ENABLE) == 0u)
        return true;

    const unsigned sync_mode =
        (timer->mode & PSX_TIMER_MODE_SYNC_MASK) >>
        PSX_TIMER_MODE_SYNC_SHIFT;
    if (sync_mode == 1u && rising) {
        timer->counter = 0u;
        timer->sync_started = true;
    } else if (sync_mode == 2u && rising) {
        timer->counter = 0u;
        timer->sync_started = true;
    } else if (sync_mode == 3u && active) {
        timer->sync_started = true;
    }
    return true;
}

static bool timer_sync_allows_clock(const PsxTimer *timer, unsigned index)
{
    if (index >= 2u ||
        (timer->mode & PSX_TIMER_MODE_SYNC_ENABLE) == 0u)
        return true;
    const unsigned sync_mode =
        (timer->mode & PSX_TIMER_MODE_SYNC_MASK) >>
        PSX_TIMER_MODE_SYNC_SHIFT;
    switch (sync_mode) {
    case 0u: return !timer->sync_signal_active;
    case 1u: return true;
    case 2u: return timer->sync_signal_active;
    default: return timer->sync_started;
    }
}

bool psx_timers_advance_clock_events(PsxTimers *timers, unsigned index,
                                     PsxTimerClock clock, uint32_t ticks,
                                     uint32_t *irq_event_count)
{
    if (!timers || index >= PSX_TIMER_COUNT ||
        clock > PSX_TIMER_CLOCK_SYSTEM_DIV8)
        return false;
    if (irq_event_count) *irq_event_count = 0u;
    PsxTimer *timer = &timers->timer[index];
    if (selected_clock(index, timer->mode) != clock ||
        !timer_sync_allows_clock(timer, index)) return true;

    while (ticks-- != 0u) {
        const uint16_t previous = timer->counter;
        timer->counter = (uint16_t)(previous + 1u);
        bool target = false;
        bool overflow = false;

        if (timer->counter == timer->target) {
            target = true;
            timer->mode |= PSX_TIMER_MODE_REACHED_TARGET;
            if ((timer->mode & PSX_TIMER_MODE_RESET_AT_TARGET) != 0u)
                timer->counter = 0u;
        }
        if (previous == 0xffffu) {
            overflow = true;
            timer->mode |= PSX_TIMER_MODE_REACHED_OVERFLOW;
        }

        const bool irq_event =
            (target && (timer->mode & PSX_TIMER_MODE_IRQ_TARGET) != 0u) ||
            (overflow && (timer->mode & PSX_TIMER_MODE_IRQ_OVERFLOW) != 0u);
        if (irq_event &&
            ((timer->mode & PSX_TIMER_MODE_IRQ_REPEAT) != 0u ||
             !timer->irq_fired)) {
            timer->irq_fired = true;
            if (irq_event_count) ++*irq_event_count;
        }
    }
    return true;
}

bool psx_timers_advance_clock(PsxTimers *timers, unsigned index,
                              PsxTimerClock clock, uint32_t ticks,
                              uint8_t *irq_sources)
{
    uint32_t event_count = 0u;
    if (irq_sources) *irq_sources = 0u;
    if (!psx_timers_advance_clock_events(timers, index, clock, ticks,
                                         &event_count))
        return false;
    if (irq_sources && event_count != 0u)
        *irq_sources = (uint8_t)(1u << index);
    return true;
}
