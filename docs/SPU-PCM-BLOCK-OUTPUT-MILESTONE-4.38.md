# SPU interleaved PCM block output — milestone 4.38

## Scope

This milestone adds a fixed-size host-side PCM block API:

`psx_spu_mix_block(PsxSpu *, int16_t *interleaved_stereo, size_t frame_capacity, size_t *frames_written)`

The buffer is interleaved signed 16-bit stereo: left sample, right sample, repeated for each frame. The function invokes the existing 24-voice mixer once per frame. If no voice contributes to a frame, that frame is explicitly written as silence. This makes the output buffer deterministic and prevents stale audio samples from leaking into later blocks.

The function validates the SPU state and output arguments, initializes `frames_written` to zero on entry, and reports the number of frames filled. A zero-capacity request is valid with a null output pointer.

## Validation

The SPU test renders a 64-frame block from a synthetic ADPCM voice, verifies non-zero left/right output, checks the silent tail after the ADPCM end marker, and covers invalid arguments and zero-length blocks. GitHub Actions compiled the project and passed all **41/41 host CTest tests**.

## Limitations

This is a host-side PCM block producer only. It does not yet connect to KallistiOS/AICA or Dreamcast audio DMA, does not select a hardware sample rate, and does not provide resampling or buffering against the Dreamcast audio clock. The current SPU voice clocks, ADSR, volume and interpolation behavior remain simplified and are not cycle-accurate. Sweep volume, noise, pitch modulation, reverb, SPU IRQ/DMA and CD-audio mixing are not implemented. No CDI playback validation has been performed.
