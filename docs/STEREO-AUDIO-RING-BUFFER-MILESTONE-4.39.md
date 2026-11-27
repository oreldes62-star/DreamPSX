# Stereo audio ring buffer — milestone 4.39

## Scope

Adds a bounded, platform-neutral interleaved stereo PCM queue between the SPU mixer and a future platform audio consumer.

## API

- `psx_audio_buffer_init` initializes the queue.
- `psx_audio_buffer_reset` empties it and resets telemetry.
- `psx_audio_buffer_write` accepts as many stereo frames as available capacity allows.
- `psx_audio_buffer_read` returns queued frames and fills any requested shortfall with silence.

The queue holds 4096 stereo frames (8192 signed 16-bit samples). It tracks frames dropped because the queue was full and frames missing when the consumer requested more data than was queued.

## Validation

The host CI builds the new module and runs `test_audio_buffer`, covering empty operations, stereo ordering, full-queue overflow accounting, wraparound, underrun silence, and reset.

## Limitations

This is a single-threaded queue abstraction, not a thread-safe audio callback implementation. It is not yet connected to the SPU render loop or a Dreamcast AICA/KallistiOS backend. No sample-rate conversion, hardware DMA, latency tuning, or real Dreamcast audio playback is implemented or validated.
