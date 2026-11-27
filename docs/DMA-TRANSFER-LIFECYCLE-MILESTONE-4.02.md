# DMA transfer lifecycle — milestone 4.02

## Delivered

- Added an explicit active-channel mask alongside the pending queue.
- Added an atomic begin operation that selects the highest-priority enabled queued channel and moves it to active state.
- Added an abort operation: a failed endpoint releases the active slot and restores the request only if CHCR still requests it.
- Unified successful channel completion so it clears CHCR request bits, retires queued/active state, and latches the DICR channel-completion flag.
- GPU channel-2 bus dispatch now claims the active slot before invoking the endpoint and aborts cleanly when the endpoint reports failure.
- Added tests for queued → active → queued (abort) and queued → active → completed transitions.

## Scope and limitations

This is lifecycle bookkeeping around the existing synchronous channel-2 GPU transfer functions. It does not yet model cycle-by-cycle transfer progress, DMA bus ownership, CPU stalls, chopping, or FIFO wait states. Channels without implemented endpoints remain pending and are not claimed by the GPU path.

## Validation

GitHub Actions host build and CTest result will be recorded after the workflow completes.
