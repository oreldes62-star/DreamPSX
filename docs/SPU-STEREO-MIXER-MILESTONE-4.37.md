# SPU stereo mixer foundation — milestone 4.37

## Scope

This milestone adds the first host-side stereo mixing entry point for the 24 PS1 SPU voices.

The new API is:

- `psx_spu_mix_sample(PsxSpu *, int16_t *left, int16_t *right)`

For each active voice, one output step:
1. advances the existing ADSR envelope;
2. obtains the next source sample through the existing pitch-aware ADPCM voice renderer;
3. applies the current ENVX level;
4. applies the voice's fixed left and right volume registers independently;
5. accumulates all voices in 64-bit intermediates and saturates the final channels to signed 16-bit PCM.

The mixer returns false when no active voice contributes a sample. Invalid pointers or an uninitialized SPU also return false.

## Validation

The SPU test now exercises an ADPCM-backed voice through the stereo mixer, verifies a non-zero left channel and an independently muted right channel, and checks invalid-argument handling. The full host CTest suite passes **41/41 tests** in GitHub Actions.

## Deliberate limitations

This is a first deterministic software mix path, not a complete hardware-accurate SPU output engine. In particular:

- volume sweep mode is not implemented; voice volume registers are treated as fixed signed values;
- pitch stepping and ADSR are simplified software clocks, not yet tied to the PS1 SPU's exact native sample scheduler;
- noise, pitch modulation, interpolation, reverb, SPU IRQ/DMA and CD-audio input mixing are not connected;
- no Dreamcast audio backend, CDI build, Flycast playback or GDEMU validation exists yet.

The milestone proves that decoded voice samples can be combined into bounded stereo PCM values on the host. It does not yet establish audible playback in a Dreamcast build or game compatibility.
