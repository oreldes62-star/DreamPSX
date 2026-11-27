# DMA transfer instrumentation — milestone 4.04

## Delivered

- Added 64-bit cumulative counters for successfully transferred DMA payload words and linked-list packet headers read.
- Manual and request/block transfers increment the payload counter only after the memory/GPU side of a word has both succeeded.
- Linked-list transfers count each successfully read packet header separately from payload words; headers are metadata and are not included in the payload count.
- Failed or partial transfers retain counts for work that actually completed before the failure.
- DMA reset clears both counters.
- Added read-only accessors and machine/bus integration assertions covering manual upload, manual readback, linked-list, request/block upload and request/block readback.

## Timing boundary

These counters measure completed transfer work, not elapsed PS1 clock cycles. No conversion to cycles is made: the current DMA path is synchronous, and exact bus occupancy, GPU FIFO stalls, CPU stalls and per-word timing still require a hardware-accurate scheduler. This instrumentation provides a trustworthy workload baseline without inventing timing values.

## Validation

Pending GitHub Actions host build and CTest for this commit.
