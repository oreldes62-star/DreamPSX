#ifndef DREAMPSX_GTE_H
#define DREAMPSX_GTE_H

#include <stdbool.h>
#include <stdint.h>

#define PSX_GTE_REGISTER_COUNT 32u

typedef struct PsxGte {
    uint32_t data[PSX_GTE_REGISTER_COUNT];
    uint32_t control[PSX_GTE_REGISTER_COUNT];
} PsxGte;

/* Reset the GTE register file to its deterministic power-on state. */
void psx_gte_reset(PsxGte *gte);

/* Read/write a GTE data or control register by its architectural index. */
bool psx_gte_read_data(const PsxGte *gte, uint8_t index, uint32_t *value);
bool psx_gte_write_data(PsxGte *gte, uint8_t index, uint32_t value);
bool psx_gte_read_control(const PsxGte *gte, uint8_t index, uint32_t *value);
bool psx_gte_write_control(PsxGte *gte, uint8_t index, uint32_t value);

#endif
