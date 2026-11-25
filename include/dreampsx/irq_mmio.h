#ifndef DREAMPSX_IRQ_MMIO_H
#define DREAMPSX_IRQ_MMIO_H

#include <stdbool.h>
#include <stdint.h>
#include "dreampsx/irq.h"

/* Little-endian CPU bus adapter for the 16-bit PS1 I_STAT/I_MASK registers.
 * Returns false for unmapped addresses or misaligned halfword/word accesses.
 * Read outputs are zero-extended to the requested access width.
 */
bool psx_irq_mmio_read8(const PsxIrqController *irq, uint32_t address, uint8_t *value);
bool psx_irq_mmio_read16(const PsxIrqController *irq, uint32_t address, uint16_t *value);
bool psx_irq_mmio_read32(const PsxIrqController *irq, uint32_t address, uint32_t *value);
bool psx_irq_mmio_write8(PsxIrqController *irq, uint32_t address, uint8_t value);
bool psx_irq_mmio_write16(PsxIrqController *irq, uint32_t address, uint16_t value);
bool psx_irq_mmio_write32(PsxIrqController *irq, uint32_t address, uint32_t value);

#endif
