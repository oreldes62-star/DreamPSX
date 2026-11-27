#ifndef DREAMPSX_BUS_H
#define DREAMPSX_BUS_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/irq.h"
#include "dreampsx/memory.h"
#include "dreampsx/gpu.h"
#include "dreampsx/timers.h"

/* CPU-facing bus. Memory and IRQ controller are owned by the machine. */
typedef struct {
    PsxMemory *memory;
    PsxIrqController *irq;
    PsxGpu *gpu;
    PsxTimers *timers;
    bool initialized;
} PsxBus;

bool psx_bus_init(PsxBus *bus, PsxMemory *memory, PsxIrqController *irq,
                  PsxGpu *gpu, PsxTimers *timers);
bool psx_bus_read8(const PsxBus *bus, uint32_t address, uint8_t *value);
bool psx_bus_read16(const PsxBus *bus, uint32_t address, uint16_t *value);
bool psx_bus_read32(const PsxBus *bus, uint32_t address, uint32_t *value);
bool psx_bus_write8(PsxBus *bus, uint32_t address, uint8_t value);
bool psx_bus_write16(PsxBus *bus, uint32_t address, uint16_t value);
bool psx_bus_write32(PsxBus *bus, uint32_t address, uint32_t value);

#endif
