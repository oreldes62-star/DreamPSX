#ifndef DREAMPSX_PSX_EXE_H
#define DREAMPSX_PSX_EXE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dreampsx/memory.h"

#define PSX_EXE_HEADER_SIZE 0x800u

typedef struct {
    uint32_t entry_pc;
    uint32_t global_pointer;
    uint32_t load_address;
    uint32_t file_size;
    uint32_t bss_address;
    uint32_t bss_size;
    uint32_t stack_base;
    uint32_t stack_offset;
    uint32_t initial_stack_pointer;
} PsxExeInfo;

/* Validates and loads a PS-X EXE into main RAM. The caller retains ownership
 * of the input buffer. Loading is rejected before memory is modified if any
 * payload/BSS range is outside physical 2 MiB RAM. */
bool psx_exe_load(PsxMemory *memory, const uint8_t *image, size_t image_size,
                  PsxExeInfo *info);

#endif
