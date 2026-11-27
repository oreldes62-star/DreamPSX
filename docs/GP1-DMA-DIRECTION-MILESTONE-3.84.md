# GP1 DMA direction and GPUSTAT request — milestone 3.84

## Scope

Implement the GP1(04h) DMA direction register and derive the related GPUSTAT fields. This is a GPU-side status foundation; it is not yet a DMA controller or a channel-2 transfer engine.

## Implemented

- `PsxGpu.dma_direction` stores GP1(04h) bits 0–1.
- GPU reset returns the direction to Off (0).
- GPUSTAT bits 29–30 mirror the selected direction.
- GPUSTAT bit 25 is derived from that direction:
  - 0 (Off): always clear.
  - 1 (FIFO/WFNF): follows the currently modeled GP0 input-ready state.
  - 2 (CPU-to-GP0/WFEP): follows the currently modeled write-ready state.
  - 3 (GPUREAD-to-CPU/RFFL): follows VRAM readback data availability.
- Software GP0 writes and GPUREAD reads remain available regardless of the selected DMA direction.

## Tests

`tests/test_gpu.c` checks all four direction encodings, GPUSTAT direction mirroring, the request bit for modes 0–3, and request assertion/deassertion around a VRAM-to-CPU readback.

## Accuracy limits

The project does not yet contain a real GP0 command FIFO. Consequently, FIFO mode (1) is approximated using the existing parser's input-ready state; mode 2 uses the current modeled write-ready state. GPU command execution is still synchronous, and the write FIFO empty/full transitions are not cycle-accurate. No DMA channel 2 registers, linked-list traversal, block transfer, chopping, or bus arbitration are implemented by this milestone. Those must be separate steps before DMA game compatibility is claimed.

Tests have been added but have not been compiled or run in this environment.
