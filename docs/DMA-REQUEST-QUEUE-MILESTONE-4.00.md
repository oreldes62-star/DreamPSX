# DMA request queue and priority take — milestone 4.00

## Delivered

- Added a seven-channel queued-request bitmask to the DMA controller.
- Added enqueue, queue-inspection, and take-next APIs.
- The scheduler takes only enabled channels and uses DPCR priority; equal priorities resolve by lower channel index.
- Requests for disabled channels remain queued until enabled.
- Successful common channel completion removes the corresponding queued entry.
- DMA reset clears the queue.
- Added regression coverage for duplicate-safe bitmask behavior, invalid channel rejection, disabled retention, priority order, empty queue, and reset.

## Scope and limitations

This is a deterministic arbitration queue foundation, not yet a cycle-accurate DMA engine. The queue is not yet automatically populated by CHCR writes or wired to every device endpoint. Channel 2 remains the only channel with implemented transfer routines. Bus arbitration, transfer timing, CPU stalls, and device-specific channels remain future work.

## Validation

Host build and CTest are run by GitHub Actions. Results will be recorded after the workflow completes.
