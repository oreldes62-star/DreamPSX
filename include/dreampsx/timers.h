#ifndef DREAMPSX_TIMERS_H
#define DREAMPSX_TIMERS_H

#include <stdbool.h>
#include <stdint.h>

#define PSX_TIMER_COUNT 3u
#define PSX_TIMER_BASE_ADDR 0x1f801100u
#define PSX_TIMER_STRIDE 0x10u

#define PSX_TIMER_MODE_RESET_AT_TARGET (1u << 3)
#define PSX_TIMER_MODE_REACHED_TARGET  (1u << 10)
#define PSX_TIMER_MODE_REACHED_OVERFLOW (1u << 11)
#define PSX_TIMER_MODE_IRQ_TARGET (1u << 4)
#define PSX_TIMER_MODE_IRQ_OVERFLOW (1u << 5)
#define PSX_TIMER_MODE_IRQ_REPEAT (1u << 6)
#define PSX_TIMER_MODE_CLOCK_SHIFT 8u

typedef enum {
    PSX_TIMER_CLOCK_SYSTEM = 0,
    PSX_TIMER_CLOCK_DOT = 1,
    PSX_TIMER_CLOCK_HBLANK = 2,
    PSX_TIMER_CLOCK_SYSTEM_DIV8 = 3
} PsxTimerClock;


typedef struct {
    uint16_t counter;
    uint16_t mode;
    uint16_t target;
    bool irq_fired;
} PsxTimer;

typedef struct {
    PsxTimer timer[PSX_TIMER_COUNT];
} PsxTimers;

void psx_timers_reset(PsxTimers *timers);
bool psx_timers_read16(PsxTimers *timers, uint32_t address,
                       uint16_t *value);
bool psx_timers_read32(PsxTimers *timers, uint32_t address,
                       uint32_t *value);
bool psx_timers_write16(PsxTimers *timers, uint32_t address,
                        uint16_t value);
bool psx_timers_write32(PsxTimers *timers, uint32_t address,
                        uint32_t value);
/* Advance one selected counter by external clock ticks. Clock-source
 * selection, synchronization and IRQ generation are intentionally separate
 * follow-up work; this primitive is deterministic and host-testable.
 */
bool psx_timers_advance(PsxTimers *timers, unsigned timer,
                        uint32_t ticks);
/* Advance only when the supplied clock matches the timer's selected source.
 * irq_sources receives a bit per timer (0..2) for IRQ events generated.
 */
bool psx_timers_advance_clock(PsxTimers *timers, unsigned timer,
                              PsxTimerClock clock, uint32_t ticks,
                              uint8_t *irq_sources);

#endif
