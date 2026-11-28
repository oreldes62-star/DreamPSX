#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "dreampsx/gpu.h"

static void init_gpu(PsxGpu *gpu, uint16_t *vram)
{
    assert(psx_gpu_init(gpu, vram,
                        PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT));
    assert(psx_gpu_gp1_write(gpu, 0x07000000u | (256u << 10u) | 16u));
}

static void advance(PsxGpu *gpu, PsxGpuVideoState *state,
                    unsigned count)
{
    for (unsigned i = 0u; i < count; ++i)
        assert(psx_gpu_video_advance_scanline(gpu, state));
}

int main(void)
{
    static uint16_t vram[PSX_GPU_VRAM_WIDTH * PSX_GPU_VRAM_HEIGHT];
    PsxGpu gpu;
    PsxGpuVideoState state;
    uint32_t status = 0u;

    init_gpu(&gpu, vram);

    /* Reset and GP1(08h) begin at line zero, outside [16, 256):
     * VBlank is already active before the first scanline advances.
     */
    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 10u)) != 0u);

    /* NTSC progressive: 263 scanlines per frame. */
    assert(psx_gpu_gp1_write(&gpu, 0x08000000u));
    assert(gpu.display.video_vblank);
    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 10u)) != 0u);
    advance(&gpu, &state, 1u);
    assert(state.scanline == 1u && !state.pal);
    assert(state.hblank_pulse);
    assert(state.vblank && !state.vblank_started);
    advance(&gpu, &state, 14u);
    assert(state.scanline == 15u && state.vblank);
    assert(state.hblank_pulse);
    advance(&gpu, &state, 1u);
    assert(state.scanline == 16u && !state.vblank);
    assert(state.vblank_ended);
    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 10u)) == 0u);
    advance(&gpu, &state, 240u);
    assert(state.scanline == 256u && state.vblank);
    advance(&gpu, &state, 7u);
    assert(state.scanline == 0u && state.field_started);
    assert(state.frame_started && state.frame_counter == 1u);
    assert(state.lines_per_field == 263u);

    /* PAL progressive: 314 scanlines per frame. */
    assert(psx_gpu_gp1_write(&gpu, 0x08000008u));
    advance(&gpu, &state, 314u);
    assert(state.scanline == 0u && state.pal);
    assert(state.lines_per_field == 314u);
    assert(state.frame_started && state.frame_counter == 1u);

    /* NTSC 480i: alternating 262/263-line fields. */
    assert(psx_gpu_gp1_write(&gpu, 0x08000024u));
    advance(&gpu, &state, 262u);
    assert(state.scanline == 0u && state.field_started);
    assert(state.field_odd && state.lines_per_field == 263u);
    assert(!state.frame_started && state.frame_counter == 0u);
    advance(&gpu, &state, 263u);
    assert(state.scanline == 0u && !state.field_odd);
    assert(state.frame_started && state.frame_counter == 1u);

    /* PAL 480i: alternating 312/313-line fields. */
    assert(psx_gpu_gp1_write(&gpu, 0x0800002cu));
    advance(&gpu, &state, 312u);
    assert(state.scanline == 0u && state.field_odd);
    assert(state.lines_per_field == 313u);
    advance(&gpu, &state, 313u);
    assert(state.scanline == 0u && !state.field_odd);
    assert(state.frame_counter == 1u);

    assert(psx_gpu_read_status(&gpu, &status));
    assert((status & (1u << 31u)) == 0u);
    assert(!psx_gpu_video_advance_scanline(NULL, &state));
    assert(!psx_gpu_video_advance_scanline(&gpu, NULL));
    puts("GPU video timing tests passed");
    return 0;
}
