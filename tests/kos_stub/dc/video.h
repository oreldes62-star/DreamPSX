#ifndef TEST_KOS_DC_VIDEO_H
#define TEST_KOS_DC_VIDEO_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { PM_RGB555 = 0, PM_RGB565 = 1 } vid_pixel_mode_t;
#define DM_320x240 0x1000

extern uint16_t *vram_s;
void vid_set_mode(int mode, vid_pixel_mode_t pixel_mode);
void vid_set_dithering(bool enable);
void vid_clear(uint8_t r, uint8_t g, uint8_t b);
void vid_waitvbl(void);

#endif
