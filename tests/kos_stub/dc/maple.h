#ifndef TEST_KOS_DC_MAPLE_H
#define TEST_KOS_DC_MAPLE_H
#include <stddef.h>
#include <stdint.h>
#define MAPLE_FUNC_CONTROLLER 0x01000000u
typedef struct maple_device { int unused; } maple_device_t;
maple_device_t *maple_enum_type(size_t n, uint32_t func);
void *maple_dev_status(maple_device_t *dev);
#endif
