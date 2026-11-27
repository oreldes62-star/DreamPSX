#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

int main(void)
{
    PsxGpu gpu;
    PsxGpuDisplay display;
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    uint32_t status = 0u;

    assert(psx_gpu_init(&gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    assert(psx_gpu_get_display(&gpu, &display));
    assert(display.start_x == 0u && display.start_y == 0u);
    assert(display.horizontal_start == 0x200u);
    assert(display.horizontal_end == 0xc00u);
    assert(display.vertical_start == 0x10u);
    assert(display.vertical_end == 0x100u);
    assert(display.mode == 0u);
    assert(display.disabled);

    /* GP1(05h): VRAM display origin (X is even-aligned). */
    assert(psx_gpu_gp1_write(&gpu, 0x05000000u | 0x155u | (0x123u << 10u)));
    /* GP1(06h): horizontal timing range. */
    assert(psx_gpu_gp1_write(&gpu, 0x06000000u | 0x260u | (0xc60u << 12u)));
    /* GP1(07h): vertical timing range. */
    assert(psx_gpu_gp1_write(&gpu, 0x07000000u | 0x20u | (0x110u << 10u)));
    /* GP1(08h): mode byte; reserved upper bits are ignored. */
    assert(psx_gpu_gp1_write(&gpu, 0x08abcdefu | 0x00000000u));

    assert(psx_gpu_get_display(&gpu, &display));
    assert(display.start_x == 0x154u);
    assert(display.start_y == 0x123u);
    assert(display.horizontal_start == 0x260u);
    assert(display.horizontal_end == 0xc60u);
    assert(display.vertical_start == 0x20u);
    assert(display.vertical_end == 0x110u);
    assert(display.mode == 0xefu);

    /* GP1(03h): enable clears GPUSTAT display-disable bit 23. */
    assert(psx_gpu_gp1_write(&gpu, 0x03000000u));
    assert(psx_gpu_get_display(&gpu, &display) && !display.disabled);
    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 23u)) == 0u);

    /* Disable sets the bit again. */
    assert(psx_gpu_gp1_write(&gpu, 0x03000001u));
    assert(psx_gpu_get_display(&gpu, &display) && display.disabled);
    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 23u)) != 0u);

    /* GP1 reset restores display defaults and disabled state. */
    assert(psx_gpu_gp1_write(&gpu, 0x00000000u));
    assert(psx_gpu_get_display(&gpu, &display));
    assert(display.start_x == 0u && display.start_y == 0u);
    assert(display.mode == 0u && display.disabled);
    assert(display.horizontal_start == 0x200u);
    assert(display.horizontal_end == 0xc00u);
    assert(display.vertical_start == 0x10u);
    assert(display.vertical_end == 0x100u);

    assert(!psx_gpu_get_display(&gpu, NULL));
    puts("GPU GP1 display register tests passed");
    return 0;
}
