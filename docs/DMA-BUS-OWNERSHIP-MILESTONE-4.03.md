# DMA bus ownership serialization — milestone 4.03

## Delivered

- The DMA controller now grants only one active channel at a time.
- `psx_dma_begin_channel()` refuses a second claim while any channel remains active.
- A completed or aborted channel releases ownership; the next eligible queued channel can then be selected by DPCR priority.
- Queue refresh excludes channels already marked active, preventing an in-flight CHCR request from reappearing as pending.
- Added regression coverage for channel handoff: channel 2 claims the bus, channel 3 remains queued through refresh, a second begin is rejected, and channel 3 can start after channel 2 retires.
- This prevents the software arbitration layer from representing simultaneous DMA transfers on a single shared bus.

## Scope and limitations

The ownership lock is logical bookkeeping, not a cycle-accurate bus arbiter. The current GPU channel still executes synchronously. Other channel device endpoints, transfer timing, CPU stalls, DMA chopping, and FIFO backpressure remain future work.

## Validation

GitHub Actions host build and CTest result will be recorded after the workflow completes.
