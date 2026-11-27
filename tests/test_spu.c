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
    int16_t mixed_left = 0;
    int16_t mixed_right = 0;
    int16_t pcm_block[128] = {0};
    size_t pcm_frames = 0u;

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

    /* An inactive SPU must overwrite every requested output frame with
     * silence; callers must never receive stale samples from a prior block. */
    {
        int16_t silent_block[6] = {123, -123, 456, -456, 789, -789};
        size_t silent_frames = 99u;
        assert(psx_spu_mix_block(&spu, silent_block, 3u, &silent_frames));
        assert(silent_frames == 3u);
        for (unsigned i = 0u; i < 6u; ++i)
            assert(silent_block[i] == 0);
        silent_frames = 99u;
        assert(!psx_spu_mix_block(&spu, NULL, 3u, &silent_frames));
        assert(silent_frames == 0u);
        assert(!psx_spu_mix_block(&spu, silent_block, 3u, NULL));
    }

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

    /* ADPCM loop-start (0x02) captures the repeat address; an end+repeat
     * block (0x05) jumps back instead of deactivating the voice. */
    {
        uint8_t loop_start[PSX_SPU_ADPCM_BLOCK_BYTES] = {0};
        uint8_t loop_end[PSX_SPU_ADPCM_BLOCK_BYTES] = {0};
        uint16_t repeat_register = 0u;
        loop_start[1] = 0x02u;
        loop_start[2] = 0x11u;
        loop_end[1] = 0x05u;
        loop_end[2] = 0x22u;
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x006u, 0x0060u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0060u));
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; i += 2u) {
            uint16_t word = (uint16_t)(loop_start[i] |
                              ((uint16_t)loop_start[i + 1u] << 8u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                                   PSX_SPU_TRANSFER_DATA_OFFSET, word));
        }
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; i += 2u) {
            uint16_t word = (uint16_t)(loop_end[i] |
                              ((uint16_t)loop_end[i + 1u] << 8u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                                   PSX_SPU_TRANSFER_DATA_OFFSET, word));
        }
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_ON_LOW_OFFSET, 0x0001u));
        for (unsigned i = 0u; i < 2u * PSX_SPU_ADPCM_BLOCK_SAMPLES; ++i)
            assert(psx_spu_voice_next_sample(&spu, 0u, &voice_sample));
        assert(psx_spu_active_voice_mask(&spu) == 0x00000001u);
        assert(spu.voices[0].repeat_address == 0x0060u);
        assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 0x00eu,
                              &repeat_register));
        assert(repeat_register == 0x0060u);
        assert(spu.voices[0].playback_address == 0x0300u);
        assert(psx_spu_voice_next_sample(&spu, 0u, &voice_sample));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0001u));
    }

    /* A software-programmed repeat address is honored even when the
     * stream contains no loop-start marker. */
    {
        uint8_t programmed_loop[PSX_SPU_ADPCM_BLOCK_BYTES] = {0};
        programmed_loop[1] = 0x05u; /* End + repeat, no loop-start. */
        programmed_loop[2] = 0x33u;
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x006u, 0x0070u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x00eu, 0x0080u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0070u));
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; i += 2u) {
            const uint16_t word = (uint16_t)(programmed_loop[i] |
                                    ((uint16_t)programmed_loop[i + 1u] << 8u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                                   PSX_SPU_TRANSFER_DATA_OFFSET, word));
        }
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_ON_LOW_OFFSET, 0x0001u));
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_SAMPLES; ++i)
            assert(psx_spu_voice_next_sample(&spu, 0u, &voice_sample));
        assert(psx_spu_active_voice_mask(&spu) == 0x00000001u);
        assert(spu.voices[0].repeat_address == 0x0080u);
        assert(spu.voices[0].playback_address == 0x0400u);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0001u));
    }

    /* Two voices keep independent software-programmed repeat addresses
     * while reaching their end+repeat blocks in interleaved calls. */
    {
        uint8_t voice0_block[PSX_SPU_ADPCM_BLOCK_BYTES] = {0};
        uint8_t voice1_block[PSX_SPU_ADPCM_BLOCK_BYTES] = {0};
        voice0_block[1] = 0x05u;
        voice0_block[2] = 0x44u;
        voice1_block[1] = 0x05u;
        voice1_block[2] = 0x55u;
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x006u, 0x00a0u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x016u, 0x00a2u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x00eu, 0x0090u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x01eu, 0x00b0u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_TRANSFER_ADDR_OFFSET, 0x00a0u));
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; i += 2u) {
            uint16_t word = (uint16_t)(voice0_block[i] |
                              ((uint16_t)voice0_block[i + 1u] << 8u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                                   PSX_SPU_TRANSFER_DATA_OFFSET, word));
        }
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; i += 2u) {
            uint16_t word = (uint16_t)(voice1_block[i] |
                              ((uint16_t)voice1_block[i + 1u] << 8u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                                   PSX_SPU_TRANSFER_DATA_OFFSET, word));
        }
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_ON_LOW_OFFSET, 0x0003u));
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_SAMPLES; ++i) {
            assert(psx_spu_voice_next_sample(&spu, 0u, &voice_sample));
            assert(psx_spu_voice_next_sample(&spu, 1u, &voice_sample));
        }
        assert(spu.voices[0].repeat_address == 0x0090u);
        assert(spu.voices[1].repeat_address == 0x00b0u);
        assert(spu.voices[0].playback_address == 0x0480u);
        assert(spu.voices[1].playback_address == 0x0580u);
        assert(psx_spu_active_voice_mask(&spu) == 0x00000003u);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0003u));
    }

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

    /* ADSR: fast linear attack reaches full scale; Key-Off enters
     * linear release and eventually clears the active voice. */
    {
        uint16_t envelope = 0u;
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x018u,
                               0x000fu)); /* voice 1 ADSR1, max sustain */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x01au,
                               0x0000u)); /* voice 1 linear, fastest release */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_ON_LOW_OFFSET, 0x0002u));
        assert(spu.voices[1].envelope_phase == PSX_SPU_ADSR_ATTACK);
        assert(psx_spu_voice_envelope_tick(&spu, 1u, &envelope));
        assert(envelope == 14336u);
        assert(psx_spu_voice_envelope_tick(&spu, 1u, &envelope));
        assert(envelope == 14336u); /* divider has not overflowed yet */
        assert(psx_spu_voice_envelope_tick(&spu, 1u, &envelope));
        assert(envelope == 28672u);
        assert(psx_spu_voice_envelope_tick(&spu, 1u, &envelope));
        assert(envelope == 28672u);
        assert(psx_spu_voice_envelope_tick(&spu, 1u, &envelope));
        assert(envelope == PSX_SPU_ENVELOPE_MAX);
        assert(spu.voices[1].envelope_phase == PSX_SPU_ADSR_SUSTAIN);
        assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + 0x01cu, &value));
        assert(value == PSX_SPU_ENVELOPE_MAX);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0002u));
        assert(spu.voices[1].envelope_phase == PSX_SPU_ADSR_RELEASE);
        assert(psx_spu_voice_envelope_tick(&spu, 1u, &envelope));
        assert(envelope == 16383u);
        assert(psx_spu_voice_envelope_tick(&spu, 1u, &envelope));
        assert(envelope == 16383u);
        assert(!psx_spu_voice_envelope_tick(&spu, 1u, &envelope));
        assert(envelope == 0u);
        assert((psx_spu_active_voice_mask(&spu) & 0x0002u) == 0u);
    }


    /* ADSR state is per voice: a fast attack must not advance a slower
     * neighboring voice's envelope or phase. */
    {
        uint16_t envelope0 = 0u;
        uint16_t envelope1 = 0u;
        psx_spu_reset(&spu);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x008u, 0x000fu));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x018u, 0x7f00u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_ON_LOW_OFFSET, 0x0003u));
        for (unsigned i = 0u; i < 5u; ++i)
            assert(psx_spu_voice_envelope_tick(&spu, 0u, &envelope0));
        assert(psx_spu_voice_envelope_tick(&spu, 1u, &envelope1));
        assert(envelope0 == PSX_SPU_ENVELOPE_MAX);
        assert(envelope1 == 0u);
        assert(spu.voices[0].envelope_phase == PSX_SPU_ADSR_DECAY ||
               spu.voices[0].envelope_phase == PSX_SPU_ADSR_SUSTAIN);
        assert(spu.voices[1].envelope_phase == PSX_SPU_ADSR_ATTACK);
        assert(spu.voices[1].envelope_counter !=
               spu.voices[0].envelope_counter);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0003u));
    }

    /* First stereo mixer path: ADPCM sample * ADSR envelope * fixed L/R
     * voice volumes. The right channel is muted independently. */
    {
        uint8_t mix_block[PSX_SPU_ADPCM_BLOCK_BYTES] = {0};
        mix_block[2] = 0x01u; /* first decoded sample = 4096 */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x000u, 0x3fffu));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x002u, 0x0000u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x004u, 0x1000u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x006u, 0x0040u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x008u, 0x000fu));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x00au, 0x0000u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0040u));
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; i += 2u) {
            const uint16_t word = (uint16_t)(mix_block[i] |
                                    ((uint16_t)mix_block[i + 1u] << 8u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                                   PSX_SPU_TRANSFER_DATA_OFFSET, word));
        }
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_ON_LOW_OFFSET, 0x0001u));
        assert(psx_spu_mix_sample(&spu, &mixed_left, &mixed_right));
        assert(mixed_left > 0);
        assert(mixed_right == 0);
        assert(spu.voices[0].envelope_level > 0u);
        assert(!psx_spu_mix_sample(NULL, &mixed_left, &mixed_right));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0001u));
    }


    /* Two full-scale voices must saturate the stereo accumulator rather
     * than wrap when their positive samples are summed. */
    {
        psx_spu_reset(&spu);
        spu.active_voices = 0x00000003u;
        for (unsigned i = 0u; i < 2u; ++i) {
            PsxSpuVoice *voice = &spu.voices[i];
            voice->volume_left = 0x3fffu;
            voice->volume_right = 0x3fffu;
            voice->volume_left_level = 32766;
            voice->volume_right_level = 32766;
            voice->pitch = 0x1000u;
            voice->envelope_level = PSX_SPU_ENVELOPE_MAX;
            voice->envelope_phase = PSX_SPU_ADSR_SUSTAIN;
            voice->block_valid = true;
            voice->decoded_samples[0] = 28672;
            voice->sample_index = 0u;
        }
        assert(psx_spu_mix_sample(&spu, &mixed_left, &mixed_right));
        assert(mixed_left == 32767);
        assert(mixed_right == 32767);
        assert(spu.voices[0].sample_index == 1u);
        assert(spu.voices[1].sample_index == 1u);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0003u));
    }

    /* All 24 voices contribute to both channels at once. Positive and
     * negative peaks must saturate symmetrically without losing voices. */
    {
        psx_spu_reset(&spu);
        spu.active_voices = 0x00ffffffu;
        for (unsigned i = 0u; i < PSX_SPU_VOICE_COUNT; ++i) {
            PsxSpuVoice *voice = &spu.voices[i];
            voice->volume_left = 0x3fffu;
            voice->volume_right = 0x3fffu;
            voice->volume_left_level = 32766;
            voice->volume_right_level = 32766;
            voice->pitch = 0x1000u;
            voice->envelope_level = PSX_SPU_ENVELOPE_MAX;
            voice->envelope_phase = PSX_SPU_ADSR_SUSTAIN;
            voice->block_valid = true;
            voice->decoded_samples[0] = 2000;
            voice->decoded_samples[1] = -2000;
            voice->sample_index = 0u;
        }
        assert(psx_spu_mix_sample(&spu, &mixed_left, &mixed_right));
        assert(mixed_left == 32767);
        assert(mixed_right == 32767);
        assert(psx_spu_active_voice_mask(&spu) == 0x00ffffffu);
        for (unsigned i = 0u; i < PSX_SPU_VOICE_COUNT; ++i)
            assert(spu.voices[i].sample_index == 1u);

        assert(psx_spu_mix_sample(&spu, &mixed_left, &mixed_right));
        assert(mixed_left == -32768);
        assert(mixed_right == -32768);
        assert(psx_spu_active_voice_mask(&spu) == 0x00ffffffu);
        for (unsigned i = 0u; i < PSX_SPU_VOICE_COUNT; ++i)
            assert(spu.voices[i].sample_index == 2u);
        psx_spu_reset(&spu);
    }

    /* Fixed-size interleaved stereo PCM blocks: active voice samples are
     * mixed, and the remainder is explicitly zero-filled after voice end. */
    {
        uint8_t pcm_adpcm[PSX_SPU_ADPCM_BLOCK_BYTES] = {0};
        pcm_adpcm[1] = 0x01u; /* End-of-sound marker */
        pcm_adpcm[2] = 0x11u;
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x000u, 0x3fffu));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x002u, 0x3fffu));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x004u, 0x1000u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x006u, 0x0050u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x008u, 0x000fu));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_TRANSFER_ADDR_OFFSET, 0x0050u));
        for (unsigned i = 0u; i < PSX_SPU_ADPCM_BLOCK_BYTES; i += 2u) {
            const uint16_t word = (uint16_t)(pcm_adpcm[i] |
                                    ((uint16_t)pcm_adpcm[i + 1u] << 8u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                                   PSX_SPU_TRANSFER_DATA_OFFSET, word));
        }
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_ON_LOW_OFFSET, 0x0001u));
        assert(psx_spu_mix_block(&spu, pcm_block, 64u, &pcm_frames));
        assert(pcm_frames == 64u);
        assert(pcm_block[0] > 0);
        assert(pcm_block[1] > 0);
        for (unsigned i = 56u; i < 128u; ++i)
            assert(pcm_block[i] == 0);
        assert(!psx_spu_mix_block(NULL, pcm_block, 64u, &pcm_frames));
        assert(pcm_frames == 0u);
        assert(psx_spu_mix_block(&spu, NULL, 0u, &pcm_frames));
        assert(pcm_frames == 0u);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_KEY_OFF_LOW_OFFSET, 0x0001u));
    }

    /* The named reverb register map matches the PS1 register layout:
     * depth, work-area base, routing and all 32 DSP parameters. */
    assert(PSX_SPU_REVERB_OUTPUT_VOLUME_LEFT_OFFSET == 0x184u);
    assert(PSX_SPU_REVERB_OUTPUT_VOLUME_RIGHT_OFFSET == 0x186u);
    assert(PSX_SPU_REVERB_ENABLE_LOW_OFFSET == 0x198u);
    assert(PSX_SPU_REVERB_ENABLE_HIGH_OFFSET == 0x19au);
    assert(PSX_SPU_REVERB_WORK_AREA_START_OFFSET == 0x1a2u);
    assert(PSX_SPU_REVERB_CONFIG_DAPF1_OFFSET == 0x1c0u);
    assert(PSX_SPU_REVERB_CONFIG_VRIN_OFFSET == 0x1feu);

    /* Reverb parameter MMIO window: all 32 halfword registers are
     * independently readable/writable before the DSP path is implemented. */
    {
        uint16_t reverb_value;
        for (unsigned offset = PSX_SPU_REVERB_REGISTER_START_OFFSET;
             offset <= PSX_SPU_REVERB_REGISTER_END_OFFSET; offset += 2u) {
            const uint16_t pattern = (uint16_t)(0x5100u | (offset >> 1u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + offset,
                                   pattern));
            assert(psx_spu_read16(&spu, PSX_SPU_BASE_ADDR + offset,
                                  &reverb_value));
            assert(reverb_value == pattern);
        }
        assert(!psx_spu_read16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_REVERB_REGISTER_END_OFFSET + 2u,
                               &reverb_value));
    }


    /* Reverb work-area cursor starts at ESA, resolves relative offsets in
     * eight-byte units, clamps below ESA, and wraps at the 512 KiB boundary. */
    {
        const uint32_t base = 0x7ff00u;
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_REVERB_WORK_AREA_START_OFFSET,
                               (uint16_t)(base / 8u)));
        assert(spu.reverb_buffer_address == base);
        assert(psx_spu_reverb_resolve_address(&spu, 0u) == base);
        assert(psx_spu_reverb_resolve_address(&spu, 2u) == base + 16u);
        assert(psx_spu_reverb_resolve_address(&spu, 0x0200u) == base);
        spu.reverb_buffer_address = PSX_SPU_REVERB_BUFFER_END_ADDRESS;
        psx_spu_reverb_advance(&spu);
        assert(spu.reverb_buffer_address == base);
        assert(psx_spu_reverb_resolve_address(&spu, 0u) == base);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_REVERB_WORK_AREA_START_OFFSET, 0u));
        assert(spu.reverb_buffer_address == 0u);
        assert(psx_spu_reverb_resolve_address(&spu, 0xffffu) == 0x7fff8u);
        assert(!psx_spu_reverb_resolve_address(NULL, 0u));
    }


    /* Reverb arithmetic uses signed Q15 products (divide by 0x8000)
     * and clamps work-RAM samples to the signed 16-bit range. */
    assert(psx_spu_reverb_multiply_q15(16384, 16384) == 8192);
    assert(psx_spu_reverb_multiply_q15(-16384, 16384) == -8192);
    assert(psx_spu_reverb_multiply_q15(32767, 32767) == 32766);
    assert(psx_spu_reverb_multiply_q15(-32768, 32767) == -32767);
    assert(psx_spu_reverb_multiply_q15(-32768, -32768) == 32768);
    assert(psx_spu_reverb_multiply_q15(0, -32768) == 0);
    assert(psx_spu_reverb_saturate(40000) == 32767);
    assert(psx_spu_reverb_saturate(-40000) == -32768);
    assert(psx_spu_reverb_saturate(1234) == 1234);
    assert(psx_spu_reverb_saturate(-1234) == -1234);


    /* First IIR reflection stage: input gain, feedback history, wall
     * coefficient and opposite-channel DIFF delay taps. */
    {
        const uint32_t cursor = 0x100u;
        const uint32_t reg_base = PSX_SPU_BASE_ADDR;
        int16_t sample;
        psx_spu_reset(&spu);
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_WORK_AREA_START_OFFSET, 0u));
        spu.reverb_buffer_address = cursor;
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_VLIN_OFFSET, 0x4000u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_VRIN_OFFSET, 0x4000u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_VWALL_OFFSET, 0u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_VIIR_OFFSET, 0x4000u));

        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_MLSAME_OFFSET, 2u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_MRSAME_OFFSET, 6u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_MLDIFF_OFFSET, 10u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_MRDIFF_OFFSET, 14u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_DLSAME_OFFSET, 4u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_DRSAME_OFFSET, 8u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_DRDIFF_OFFSET, 12u));
        assert(psx_spu_write16(&spu, reg_base +
            PSX_SPU_REVERB_CONFIG_DLDIFF_OFFSET, 16u));

        spu.ram[0x10eu] = 0xc8u; spu.ram[0x10fu] = 0x00u;
        spu.ram[0x12eu] = 0x90u; spu.ram[0x12fu] = 0x01u;
        spu.ram[0x14eu] = 0x58u; spu.ram[0x14fu] = 0x02u;
        spu.ram[0x16eu] = 0x20u; spu.ram[0x16fu] = 0x03u;

        assert(psx_spu_reverb_process_reflections(&spu, 1000, 2000));
        assert(psx_spu_reverb_read16(&spu, 2u, &sample));
        assert(sample == 350);
        assert(psx_spu_reverb_read16(&spu, 6u, &sample));
        assert(sample == 700);
        assert(psx_spu_reverb_read16(&spu, 10u, &sample));
        assert(sample == 550);
        assert(psx_spu_reverb_read16(&spu, 14u, &sample));
        assert(sample == 900);
        assert(!psx_spu_reverb_process_reflections(NULL, 0, 0));
        psx_spu_reset(&spu);
    }

    /* Reverb sample accesses are signed little-endian halfwords and remain
     * inside the ESA..7FFFEh work area, including at the final RAM word. */
    {
        int16_t sample = 0;
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                               PSX_SPU_REVERB_WORK_AREA_START_OFFSET,
                               0xfffeu));
        assert(psx_spu_reverb_write16(&spu, 0u, (int16_t)-12345));
        assert(spu.ram[0x7fff0u] == 0xc7u);
        assert(spu.ram[0x7fff1u] == 0xcfu);
        assert(psx_spu_reverb_read16(&spu, 0u, &sample));
        assert(sample == -12345);
        assert(psx_spu_reverb_write16(&spu, 0xffffu, (int16_t)23456));
        assert(psx_spu_reverb_read16(&spu, 0xffffu, &sample));
        assert(sample == 23456);
        assert(!psx_spu_reverb_read16(&spu, 0u, NULL));
        assert(!psx_spu_reverb_write16(NULL, 0u, 1));
        psx_spu_reset(&spu);
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
    assert(value == spu.voices[0].envelope_level);
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
    /* Four-tap stereo comb stage: independent channel delay addresses,
     * shared Q15 coefficients, wide accumulation and final saturation. */
    {
        int16_t comb_left = 0;
        int16_t comb_right = 0;
        const uint16_t left_offsets[4] = {
            PSX_SPU_REVERB_CONFIG_MLCOMB1_OFFSET,
            PSX_SPU_REVERB_CONFIG_MLCOMB2_OFFSET,
            PSX_SPU_REVERB_CONFIG_MLCOMB3_OFFSET,
            PSX_SPU_REVERB_CONFIG_MLCOMB4_OFFSET
        };
        const uint16_t right_offsets[4] = {
            PSX_SPU_REVERB_CONFIG_MRCOMB1_OFFSET,
            PSX_SPU_REVERB_CONFIG_MRCOMB2_OFFSET,
            PSX_SPU_REVERB_CONFIG_MRCOMB3_OFFSET,
            PSX_SPU_REVERB_CONFIG_MRCOMB4_OFFSET
        };
        const uint16_t volume_offsets[4] = {
            PSX_SPU_REVERB_CONFIG_VCOMB1_OFFSET,
            PSX_SPU_REVERB_CONFIG_VCOMB2_OFFSET,
            PSX_SPU_REVERB_CONFIG_VCOMB3_OFFSET,
            PSX_SPU_REVERB_CONFIG_VCOMB4_OFFSET
        };
        psx_spu_reset(&spu);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_WORK_AREA_START_OFFSET, 0u));
        for (unsigned i = 0u; i < 4u; ++i) {
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + left_offsets[i],
                                   (uint16_t)i));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + right_offsets[i],
                                   (uint16_t)(i + 4u)));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + volume_offsets[i],
                                   0x4000u));
            assert(psx_spu_reverb_write16(&spu, (uint16_t)i,
                                           (int16_t)((i + 1u) * 1000u)));
            assert(psx_spu_reverb_write16(&spu, (uint16_t)(i + 4u),
                                           (int16_t)(-((int)i + 1) * 1000)));
        }
        assert(psx_spu_reverb_process_comb(&spu, &comb_left, &comb_right));
        assert(comb_left == 5000);
        assert(comb_right == -5000);
        /* The comb stage is a read/compute operation; it must not advance
         * the shared reverb work-area cursor or alter its delay samples. */
        assert(spu.reverb_buffer_address == 0u);
        assert(psx_spu_reverb_write16(&spu, 0u, 30000));
        assert(psx_spu_reverb_write16(&spu, 1u, 30000));
        assert(psx_spu_reverb_write16(&spu, 2u, 30000));
        assert(psx_spu_reverb_write16(&spu, 3u, 30000));
        assert(psx_spu_reverb_process_comb(&spu, &comb_left, &comb_right));
        assert(comb_left == 32767);
        assert(!psx_spu_reverb_process_comb(NULL, &comb_left, &comb_right));
        assert(!psx_spu_reverb_process_comb(&spu, NULL, &comb_right));
        psx_spu_reset(&spu);
    }

    /* Cascaded APF1/APF2 stages, including feedback writes and signed
     * stereo delayed samples. */
    {
        int16_t apf_left = 0;
        int16_t apf_right = 0;
        int16_t sample = 0;
        psx_spu_reset(&spu);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_WORK_AREA_START_OFFSET, 0u));
        spu.reverb_buffer_address = 0x100u;
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_CONFIG_DAPF1_OFFSET, 4u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_CONFIG_DAPF2_OFFSET, 2u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_CONFIG_VAPF1_OFFSET, 0x4000u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_CONFIG_VAPF2_OFFSET, 0x4000u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_CONFIG_MLAPF1_OFFSET, 20u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_CONFIG_MRAPF1_OFFSET, 44u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_CONFIG_MLAPF2_OFFSET, 30u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_CONFIG_MRAPF2_OFFSET, 54u));
        assert(psx_spu_reverb_write16(&spu, 16u, 2000));
        assert(psx_spu_reverb_write16(&spu, 40u, -2000));
        assert(psx_spu_reverb_write16(&spu, 28u, 1000));
        assert(psx_spu_reverb_write16(&spu, 52u, -1000));
        assert(psx_spu_reverb_process_allpass(&spu, 10000, -10000,
                                               &apf_left, &apf_right));
        assert(apf_left == 4500);
        assert(apf_right == -4500);
        assert(psx_spu_reverb_read16(&spu, 20u, &sample));
        assert(sample == 9000);
        assert(psx_spu_reverb_read16(&spu, 44u, &sample));
        assert(sample == -9000);
        assert(psx_spu_reverb_read16(&spu, 30u, &sample));
        assert(sample == 6500);
        assert(psx_spu_reverb_read16(&spu, 54u, &sample));
        assert(sample == -6500);
        assert(spu.reverb_buffer_address == 0x100u);
        assert(!psx_spu_reverb_process_allpass(NULL, 0, 0,
                                                &apf_left, &apf_right));
        assert(!psx_spu_reverb_process_allpass(&spu, 0, 0,
                                                NULL, &apf_right));
        psx_spu_reset(&spu);
    }

    /* Full wet-path orchestration: disabled mode is transparent and does
     * not advance ESA; enabled mode runs the stages and advances once. */
    {
        int16_t wet_left = 123;
        int16_t wet_right = -123;
        psx_spu_reset(&spu);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_REVERB_WORK_AREA_START_OFFSET, 0u));
        spu.reverb_buffer_address = 0x200u;
        assert(psx_spu_reverb_process_sample(&spu, 12000, -12000,
                                              &wet_left, &wet_right));
        assert(wet_left == 0 && wet_right == 0);
        assert(spu.reverb_buffer_address == 0x200u);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
            PSX_SPU_CONTROL_OFFSET, 0x0080u));
        assert(psx_spu_write16(&spu, PSX_SPU_REVERB_OUTPUT_VOLUME_LEFT_OFFSET +
            PSX_SPU_BASE_ADDR, 0x7fffu));
        assert(psx_spu_write16(&spu, PSX_SPU_REVERB_OUTPUT_VOLUME_RIGHT_OFFSET +
            PSX_SPU_BASE_ADDR, 0x7fffu));
        assert(psx_spu_reverb_process_sample(&spu, 12000, -12000,
                                              &wet_left, &wet_right));
        assert(wet_left == 0 && wet_right == 0);
        assert(spu.reverb_buffer_address == 0x202u);
        assert(!psx_spu_reverb_process_sample(NULL, 0, 0,
                                               &wet_left, &wet_right));
        assert(!psx_spu_reverb_process_sample(&spu, 0, 0,
                                               NULL, &wet_right));
        psx_spu_reset(&spu);
    }

    /* Per-voice reverb routing: voice 0 uses the low bitmap and voice
     * 23 uses bit 7 of the high bitmap. Only selected voices reach the
     * reflection input; dry mixing remains active in both cases. */
    {
        const unsigned routed_voices[2] = {0u, 23u};
        for (unsigned test = 0u; test < 2u; ++test) {
            const unsigned voice_index = routed_voices[test];
            const uint32_t voice_bit = 1u << voice_index;
            const uint32_t voice_register =
                voice_index * PSX_SPU_VOICE_REGISTER_BYTES;
            int16_t dry_left = 0;
            int16_t dry_right = 0;
            const uint32_t reflection_address = 0x100u;

            psx_spu_reset(&spu);
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_WORK_AREA_START_OFFSET, 0x0020u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_CONTROL_OFFSET, PSX_SPU_REVERB_MASTER_ENABLE));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_VLIN_OFFSET, 0x7fffu));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_VRIN_OFFSET, 0x7fffu));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_VIIR_OFFSET, 0x7fffu));
            /* Keep APF writes away from the reflection destination. */
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_MLAPF1_OFFSET, 0x0020u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_MRAPF1_OFFSET, 0x0024u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_MLAPF2_OFFSET, 0x0028u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_MRAPF2_OFFSET, 0x002cu));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                voice_register + 0x00u, 0x3fffu));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                voice_register + 0x02u, 0x3fffu));

            spu.active_voices = voice_bit;
            spu.voices[voice_index].current_sample = 4096;
            spu.voices[voice_index].current_sample_valid = true;
            spu.voices[voice_index].envelope_level = PSX_SPU_ENVELOPE_MAX;
            spu.voices[voice_index].envelope_phase = PSX_SPU_ADSR_SUSTAIN;
            spu.voices[voice_index].pitch = 0u;

            /* Without the voice's send bit, the reverb work area receives
             * silence even though the dry channel still produces audio. */
            assert(psx_spu_mix_sample(&spu, &dry_left, &dry_right));
            assert(dry_left > 0 && dry_right > 0);
            assert(spu.ram[reflection_address] == 0u);
            assert(spu.ram[reflection_address + 1u] == 0u);

            /* Re-run from a clean state with the correct low/high send bit. */
            psx_spu_reset(&spu);
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_WORK_AREA_START_OFFSET, 0x0020u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_CONTROL_OFFSET, PSX_SPU_REVERB_MASTER_ENABLE));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_VLIN_OFFSET, 0x7fffu));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_VRIN_OFFSET, 0x7fffu));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_VIIR_OFFSET, 0x7fffu));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_MLAPF1_OFFSET, 0x0020u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_MRAPF1_OFFSET, 0x0024u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_MLAPF2_OFFSET, 0x0028u));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                PSX_SPU_REVERB_CONFIG_MRAPF2_OFFSET, 0x002cu));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                voice_register + 0x00u, 0x3fffu));
            assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                voice_register + 0x02u, 0x3fffu));
            spu.active_voices = voice_bit;
            spu.voices[voice_index].current_sample = 4096;
            spu.voices[voice_index].current_sample_valid = true;
            spu.voices[voice_index].envelope_level = PSX_SPU_ENVELOPE_MAX;
            spu.voices[voice_index].envelope_phase = PSX_SPU_ADSR_SUSTAIN;
            if (voice_index < 16u) {
                assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                    PSX_SPU_REVERB_ENABLE_LOW_OFFSET,
                    (uint16_t)voice_bit));
            } else {
                assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR +
                    PSX_SPU_REVERB_ENABLE_HIGH_OFFSET,
                    (uint16_t)(voice_bit >> 16u)));
            }
            assert(psx_spu_mix_sample(&spu, &dry_left, &dry_right));
            assert(spu.ram[reflection_address] != 0u ||
                   spu.ram[reflection_address + 1u] != 0u);
            assert(spu.reverb_buffer_address == 0x102u);
        }
        psx_spu_reset(&spu);
    }

    /* Direct volume is signed 15-bit doubled data. Sweep mode advances
     * independently on left and right at the SPU sample cadence. */
    {
        psx_spu_reset(&spu);
        assert(psx_spu_decode_direct_volume(0x3fffu) == 32766);
        assert(psx_spu_decode_direct_volume(0x4000u) == -32768);
        assert(psx_spu_decode_direct_volume(0x7fffu) == -2);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR, 0x2000u));
        assert(spu.voices[0].volume_left_level == 16384);
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR + 0x002u, 0x1000u));
        assert(spu.voices[0].volume_right_level == 8192);
        /* Shift 11, step 0: +7 per output sample on the left. */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR, 0x8000u | (11u << 2u)));
        assert(psx_spu_voice_volume_tick(&spu, 0u));
        assert(spu.voices[0].volume_left_level == 16391);
        assert(spu.voices[0].volume_right_level == 8192);
        /* Negative-phase increase moves toward negative full scale. */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR,
                               0x8000u | 0x1000u | (11u << 2u)));
        assert(psx_spu_voice_volume_tick(&spu, 0u));
        assert(spu.voices[0].volume_left_level == 16383);
        /* The all-ones rate encoding is frozen and must not step. */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR,
                               0xffffu));
        assert(psx_spu_voice_volume_tick(&spu, 0u));
        assert(spu.voices[0].volume_left_level == 16383);

        /* Shift 12 halves the rate accumulator: two samples per step. */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR, 0x2000u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR,
                               0x8000u | (12u << 2u)));
        assert(psx_spu_voice_volume_tick(&spu, 0u));
        assert(spu.voices[0].volume_left_level == 16384);
        assert(psx_spu_voice_volume_tick(&spu, 0u));
        assert(spu.voices[0].volume_left_level == 16391);

        /* Exponential decrease scales the -8 step by current / 0x8000. */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR, 0x2000u));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR,
                               0x8000u | 0x4000u | 0x2000u |
                               (11u << 2u)));
        assert(psx_spu_voice_volume_tick(&spu, 0u));
        assert(spu.voices[0].volume_left_level == 16380);

        /* A sweep stops when its positive target is reached. */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR, 0x3fffu));
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR,
                               0x8000u | (11u << 2u)));
        assert(psx_spu_voice_volume_tick(&spu, 0u));
        assert(spu.voices[0].volume_left_level == 32767);
        assert(!spu.voices[0].volume_left_sweep_active);

        /* The maximum shift/step encoding is an explicitly frozen sweep. */
        assert(psx_spu_write16(&spu, PSX_SPU_BASE_ADDR, 0xffffu));
        assert(!spu.voices[0].volume_left_sweep_active);
        assert(!psx_spu_voice_volume_tick(NULL, 0u));
        assert(!psx_spu_voice_volume_tick(&spu, PSX_SPU_VOICE_COUNT));
        psx_spu_reset(&spu);
    }

    puts("DreamPSX SPU register/RAM tests: OK");
    return 0;
}
