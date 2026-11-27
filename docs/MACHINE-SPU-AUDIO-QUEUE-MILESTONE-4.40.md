# Machine SPU audio queue integration — milestone 4.40

## Scope

Connects the existing SPU stereo PCM block mixer to the machine-owned bounded audio queue introduced in milestone 4.39.

## API

- `psx_machine_render_audio(machine, frame_count, &frames_queued)` renders SPU output in chunks of at most 128 stereo frames and appends the accepted frames to the machine queue.
- `psx_machine_read_audio(machine, output, frame_capacity)` reads queued interleaved stereo PCM and zero-fills any shortfall.
- Machine initialization creates the queue; machine reset empties it and resets its telemetry.

Small stack chunks avoid allocating a large temporary PCM block on the Dreamcast target. If the consumer queue is full, excess rendered frames are dropped and counted by the queue rather than blocking emulation.

## Validation

The host CI test suite passes 42/42 tests. Machine tests verify render-to-queue flow, zero output when no SPU voices are active, silence fill on underrun, overflow accounting, full-buffer reads, and reset behavior.

## Limitations

The caller currently chooses when and how many frames to render; this API is not yet synchronized to PS1 CPU cycles, GPU refresh, or a fixed audio sample clock. The queue is single-threaded and has no KallistiOS/AICA callback or DMA integration. There is no Dreamcast hardware playback validation, resampling, latency control, or complete SPU hardware timing model.
