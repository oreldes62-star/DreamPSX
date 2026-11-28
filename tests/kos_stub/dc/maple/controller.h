#ifndef TEST_KOS_DC_MAPLE_CONTROLLER_H
#define TEST_KOS_DC_MAPLE_CONTROLLER_H
#include <stdint.h>
#define CONT_C (1u << 0u)
#define CONT_B (1u << 1u)
#define CONT_A (1u << 2u)
#define CONT_START (1u << 3u)
#define CONT_DPAD_UP (1u << 4u)
#define CONT_DPAD_DOWN (1u << 5u)
#define CONT_DPAD_LEFT (1u << 6u)
#define CONT_DPAD_RIGHT (1u << 7u)
#define CONT_Z (1u << 8u)
#define CONT_Y (1u << 9u)
#define CONT_X (1u << 10u)
typedef struct { uint32_t buttons; int ltrig; int rtrig; } cont_state_t;
#endif
