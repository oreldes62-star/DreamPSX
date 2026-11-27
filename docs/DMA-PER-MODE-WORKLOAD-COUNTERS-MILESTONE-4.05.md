# DMA per-mode workload counters — milestone 4.05

## Delivered

- Added cumulative 64-bit payload counters for manual, request/block, and linked-list GPU DMA.
- Kept the aggregate payload-word counter as the sum of all successfully completed payload words.
- Linked-list packet headers remain a separate metadata counter and are not included in payload totals.
- Counters advance only after the corresponding memory/GPU word operation succeeds.
- DMA reset clears all aggregate and per-mode counters.
- Added integration checks for each mode and a reconciliation assertion proving that the three mode counters sum to the aggregate.

## Timing boundary

These counters describe completed workload by DMA mode. They are not elapsed cycles, bus occupancy, or throughput estimates. The transfer engine remains synchronous; cycle conversion must wait for an explicit hardware-timing model.

## Validation

Pending GitHub Actions host build and CTest.
