# GPUSTAT readiness state — milestone 3.83

## Goal

Expose the GPU command/readback readiness state through GPUSTAT instead of returning only the stored mode bits.

## Implemented behavior

The GPU status read now derives bits 26–28 from the active GP0 parser/transfer state:

- Bit 26 (ready to receive a command word) is set when no command parameters or image transfer are pending.
- Bit 27 (VRAM-to-CPU data available) is set while a C0h readback transfer still has unread pixels.
- Bit 28 (ready to receive a DMA block) follows the currently modeled command-input readiness.
- An incomplete GP0 command clears bits 26 and 28 while its parameters are pending.
- Starting a VRAM-to-CPU transfer clears bits 26 and 28 and asserts bit 27; consuming the final data word clears bit 27 and restores command readiness.
- GP1(01h) command-buffer reset discards the incomplete parser state and restores readiness.

## Tests added

The GPU tests check initial readiness, the incomplete-command state, reset recovery, and VRAM readback readiness before and after the final data word.

## Explicit limitations

This milestone does not add GPU execution latency, a hardware FIFO, DMA direction GP1(04h), or DMA request bit 25. The current renderer still executes drawing commands synchronously. Bit 28 is therefore a readiness approximation based on the implemented parser/transfer state, not a cycle-accurate FIFO-empty signal. DMA support must be added before claiming complete DMA status behavior.

Tests have been added but have not been compiled or run in this environment.
