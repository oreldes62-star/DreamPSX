#ifndef DREAMPSX_IRQ_H
#define DREAMPSX_IRQ_H

#include <stdbool.h>
#include <stdint.h>

#define PSX_IRQ_SOURCE_COUNT 11u
#define PSX_IRQ_VALID_MASK 0x07ffu
#define PSX_IRQ_STAT_ADDR 0x1f801070u
#define PSX_IRQ_MASK_ADDR 0x1f801074u

typedef struct {
    uint16_t pending;
    uint16_t mask;
    uint16_t source_level;
} PsxIrqController;

void psx_irq_reset(PsxIrqController *irq);
void psx_irq_set_line(PsxIrqController *irq, unsigned source, bool asserted);
uint16_t psx_irq_read_stat(const PsxIrqController *irq);
uint16_t psx_irq_read_mask(const PsxIrqController *irq);
void psx_irq_write_stat(PsxIrqController *irq, uint16_t value);
void psx_irq_write_mask(PsxIrqController *irq, uint16_t value);
bool psx_irq_output(const PsxIrqController *irq);

#endif
