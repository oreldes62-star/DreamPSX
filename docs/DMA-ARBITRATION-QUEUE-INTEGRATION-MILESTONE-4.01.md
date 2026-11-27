# DMA arbitration queue integration — milestone 4.01

## Delivered

- Added `psx_dma_refresh_queue()` to rebuild the queue from the authoritative CHCR request state.
- Queue refresh adds newly armed requests and removes requests whose CHCR start conditions have been cancelled or completed.
- The bus now refreshes the queue before GPU DMA dispatch and uses the DPCR priority selector.
- Channel 2 GPU transfers execute only when channel 2 is the highest-priority enabled queued request.
- Requests belonging to channels without implemented device endpoints remain queued; the GPU path does not consume them.
- The queue is refreshed again after a transfer attempt, so an incomplete request remains visible.

## Scope and limitations

This integrates the arbitration foundation into the existing synchronous bus path; it is not yet a cycle-accurate DMA scheduler. Only channel 2 has a transfer endpoint. Other channels can block a lower-priority GPU request until their endpoint is implemented. Arbitration currently runs when software writes CHCR or DPCR; it does not yet model continuous bus-slot scheduling, CPU stalls, DMA chopping, or device FIFO backpressure.

## Validation

GitHub Actions host build and CTest result will be recorded after the workflow completes.
