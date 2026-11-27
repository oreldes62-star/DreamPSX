#include "dreampsx/spu.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    PsxSpu spu;
    uint16_t value;
    uint8_t byte;
    uint8_t adpcm_block[PSX_SPU_ADPCM_BLOCK_BYTES] = {0};
    int16_t adpcm_samples[PSX_SPU_ADPCM_BLOCK_SAMPLES];
    PsxSpuAdpcmState adpcm_state;
    uint8_t adpcm_flags = 0u;
    int16_t voice_sample = 0;
    int16_t rendered_sample = 0;

    psx_spu_adpcm_reset(&adpcm_state);
    adpcm_block[1] = 0x05u;
    adpcm_block[2] = 0x71u; /* low nibble +1, high nibble +7 */
    adpcm_block[3] = 0x0fu; /* low nibble -1, high nibble zero */
    assert(psx_spu_adpcm_decode_block(&adpcm_state, adpcm_block,
                                      adpcm_samples, &adpcm_flags));
    assert(adpcm_flags == 0x05u);
    assert(adpcm_samples[0] == 4096);
    assert(adpcm_samples[1] == 28672);
    assert(adpcm_samples[2] == -4096);
    assert(adpcm_samples[3] == 0);
    assert(adpcm_state.history_1 == 0);
    assert(adpcm_state.history_2 == 0);

    /* Invalid filter/shift headers fail without changing decoder history. */
    {
        PsxSpuAdpcmState before = adpcm_state;
        adpcm_block[0] = 0x50u;
        assert(!psx_spu_adpcm_decode_block(&adpcm_state, adpcm_block,
                                           adpcm_samples, &adpcm_flags));
        assert(adpcm_state.history_1 == before.history_1);
        assert(adpcm_state.history_2 == before.history_2);
        adpcm_block[0] = 0x0du;
        assert(!psx_spu_adpcm_decode_block(&adpcm_state, adpcm_block,
                                           adpcm_samples, &adpcm_flags));
        assert(adpcm_state.history_1 == before.history_1);
        assert(adpcm_state.history_2 == before.history_2);
        adpcm_block[0] = 0u;
    }

    assert(psx_spu_init(&spu));
    assert(spu.initialized);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR, 0x1234u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR, &value));
    assert(value == 0x1234u);

    /* Voice register space is ordinary 16-bit register storage. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x0e0u, 0x7fffu));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 0x0e0u, &value));
    assert(value == 0x7fffu);

    /* A keyed-on voice fetches, decodes and consumes a RAM ADPCM block. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x006u, 0x0020u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0020u));
    adpcm_block[0] = 0u;
    adpcm_block[1] = 0x01u; /* End-of-sound marker on this block. */
    adpcm_block[2] = 0x71u;
    adpcm_block[3] = 0x0fu;
    for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; i += 2u) {
        const uint16_t word = (uint16_t)(adpcm_block[i] |
                                ((uint16_t)adpcm_block[i + 1u] << 8u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_TRANSFER_DATA_OFFSET, word));
    }
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_ON_LOW_OFFSET, 0x0001u));
    assert(psx_spu_voice_next_sample(&spu, 0u, &voice_sample));
    assert(voice_sample == 4096);
    for (unsigned i = 1u; i < PSX_SPU_ADPCM_BLOCK_SAMPLES; ++i)
        assert(psx_spu_voice_next_sample(&spu, 0u, &voice_sample));
    assert(psx_spu_active_voice_mask(&spu) == 0u);
    assert(!psx_spu_voice_next_sample(&spu, 0u, &voice_sample));

    /* Pitch phase holds/repeats source samples below 1x and advances
     * multiple decoded samples above 1x. */
    {
        uint8_t pitch_block[PSX_SPU_ADPCM_BLOCK_BYTES] = {0};
        pitch_block[2] = 0x21u; /* decoded samples: 4096, 8192 */
        pitch_block[3] = 0x43u; /* then 12288, 16384 */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x004u, 0x0800u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x006u, 0x0030u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0030u));
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; i += 2u) {
            const uint16_t word = (uint16_t)(pitch_block[i] |
                                    ((uint16_t)pitch_block[i + 1u] << 8u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                                   PSX_SPU_TRANSFER_DATA_OFFSET, word));
        }
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_ON_LOW_OFFSET, 0x0001u));
        assert(psx_spu_voice_render_sample(&spu, 0u, &rendered_sample));
        assert(rendered_sample == 4096);
        assert(psx_spu_voice_render_sample(&spu, 0u, &rendered_sample));
        assert(rendered_sample == 4096);
        assert(psx_spu_voice_render_sample(&spu, 0u, &rendered_sample));
        assert(rendered_sample == 8192);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x004u, 0x2000u));
        assert(psx_spu_voice_render_sample(&spu, 0u, &rendered_sample));
        assert(rendered_sample == 16384);
        assert(spu.voices[0].pitch_phase == 0u);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0001u));
    }

    /* Typed voice registers expose 24 independent parameter banks. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x000u, 0x1111u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x002u, 0x2222u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x004u, 0xffffu));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x006u, 0x0123u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x008u, 0x4567u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x00au, 0x89abu));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x00eu, 0x0321u));
    assert(spu.voices[0].volume_left == 0x1111u);
    assert(spu.voices[0].volume_right == 0x2222u);
    assert(spu.voices[0].pitch == 0x3fffu);
    assert(spu.voices[0].start_address == 0x0123u);
    assert(spu.voices[0].adsr1 == 0x4567u);
    assert(spu.voices[0].adsr2 == 0x89abu);
    assert(spu.voices[0].repeat_address == 0x0321u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x00cu, 0x7777u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 0x00cu, &value));
    assert(value == 0u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x170u, 0x5a5au));
    assert(spu.voices[23].volume_left == 0x5a5au);

    /* Key-on/off registers update the 24-voice active mask and read as zero. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_ON_LOW_OFFSET, 0x8001u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_ON_HIGH_OFFSET, 0x0081u));
    assert(psx_spu_active_voice_mask(&spu) == 0x00818001u);
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_KEY_ON_LOW_OFFSET, &value));
    assert(value == 0u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0001u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_KEY_OFF_HIGH_OFFSET, 0x0080u));
    assert(psx_spu_active_voice_mask(&spu) == 0x00018000u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_CONTROL_OFFSET, 0x1234u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_CONTROL_OFFSET, &value));
    assert(value == 0x1234u);

    /* Transfer address is expressed in 8-byte units; data port advances
     * by one 16-bit word for each access. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0010u));
    assert(spu.transfer_address == 0x80u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0xabcd));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0x1357u));
    assert(spu.transfer_address == 0x84u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0010u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_TRANSFER_DATA_OFFSET, &value));
    assert(value == 0xabcdu);
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_TRANSFER_DATA_OFFSET, &value));
    assert(value == 0x1357u);

    /* Byte accesses preserve the other byte in a register. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x20u, 0x1122u));
    assert(psx_spu_write8(&spu, PSX_SPU_BASE_ADDR + 0x21u, 0xaau));
    assert(psx_spu_read8(&spu, PSX_SPU_BASE_ADDR + 0x20u, &byte));
    assert(byte == 0x22u);
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 0x20u, &value));
    assert(value == 0xaa22u);

    /* The FIFO-like transfer port is intentionally word-only. */
    assert(!psx_spu_read8(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_TRANSFER_DATA_OFFSET, &byte));
    assert(!psx_spu_write8(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0x12u));

    /* RAM wraps at 512 KiB. */
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0xffffu));
    assert(spu.transfer_address == PSX_SPU_RAM_BYTES - 8u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0x55aau));
    assert(spu.transfer_address == PSX_SPU_RAM_BYTES - 6u);

    /* Reset clears registers, RAM and transfer position. */
    psx_spu_reset(&spu);
    assert(spu.transfer_address == 0u);
    assert(psx_spu_active_voice_mask(&spu) == 0u);
    assert(spu.voices[0].volume_left == 0u);
    assert(spu.voices[0].pitch == 0u);
    assert(spu.voices[0].adsr1 == 0u);
    assert(spu.voices[23].volume_left == 0u);
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR, &value));
    assert(value == 0u);
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_DATA_OFFSET, 0x9876u));
    assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                           PSX_SPU_TRANSFER_ADDR_OFFSET, 0u));
    assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                          PSX_SPU_TRANSFER_DATA_OFFSET, &value));
    assert(value == 0x9876u);

    assert(!psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 1u, &value));
    assert(!psx_spu_read16(&spu, 0x1f801800u, &value));
    puts("DreamPSX SPU register/RAM tests: OK");
    return 0;
}
