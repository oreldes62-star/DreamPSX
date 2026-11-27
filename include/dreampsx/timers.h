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

typedef struct {
    uint16_t counter;
    uint16_t mode;
    uint16_t target;
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

#endif
