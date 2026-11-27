#ifndef DREAMPSX_BOOT_CONFIG_H
#define DREAMPSX_BOOT_CONFIG_H
#include <stdbool.h>
#include <stddef.h>
#include "dreampsx/iso9660.h"
typedef struct { char boot_path[PSX_ISO9660_PATH_MAX]; bool boot2; } PsxBootConfig;
bool psx_boot_config_parse(const char *text, size_t size, PsxBootConfig *config);
bool psx_boot_config_resolve(const PsxIso9660 *iso, PsxBootConfig *config, PsxIso9660File *executable);
#endif
