#ifndef DREAMPSX_GTE_H
#define DREAMPSX_GTE_H

#include <stdbool.h>
#include <stdint.h>

#define PSX_GTE_REGISTER_COUNT 32u

typedef struct PsxGte {
    uint32_t data[PSX_GTE_REGISTER_COUNT];
    uint32_t control[PSX_GTE_REGISTER_COUNT];
    uint32_t pending_data[PSX_GTE_REGISTER_COUNT];
    uint32_t pending_control[PSX_GTE_REGISTER_COUNT];
    uint8_t busy_cycles;
    bool pending;
} PsxGte;

/* Reset the GTE register file to its deterministic power-on state. */
void psx_gte_reset(PsxGte *gte);

/* Execute one COP2 command immediately (useful for isolated arithmetic tests). */
bool psx_gte_execute(PsxGte *gte, uint32_t instruction);
/* Queue a timed command; results become visible when its latency expires. */
bool psx_gte_issue(PsxGte *gte, uint32_t instruction);
/* Advance the GTE clock and commit a completed command result. */
void psx_gte_advance_cycles(PsxGte *gte, uint32_t cycles);
uint8_t psx_gte_command_cycles(uint32_t instruction);
uint8_t psx_gte_busy_cycles(const PsxGte *gte);

/* Read/write a GTE data or control register by its architectural index. */
bool psx_gte_read_data(const PsxGte *gte, uint8_t index, uint32_t *value);
bool psx_gte_write_data(PsxGte *gte, uint8_t index, uint32_t value);
bool psx_gte_read_control(const PsxGte *gte, uint8_t index, uint32_t *value);
bool psx_gte_write_control(PsxGte *gte, uint8_t index, uint32_t value);

#endif
