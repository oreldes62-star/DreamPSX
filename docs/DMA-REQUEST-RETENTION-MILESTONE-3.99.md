# DMA request retention and arbitration separation — milestone 3.99

## Delivered

- Changed `psx_dma_pending_mask()` to report CHCR requests independently of the DPCR channel-enable bit.
- Kept the DPCR enable check in `psx_dma_select_channel()`, where arbitration eligibility belongs.
- Added a regression test proving that a disabled channel's request remains visible, is not selected while disabled, and becomes selectable immediately after DPCR enables it.
- Preserved START, manual TRIGGER, and reserved sync-mode filtering.

## Why this matters

Software may program CHCR before enabling a DMA channel in DPCR. Request discovery must not erase or hide that request merely because arbitration is temporarily blocked. Separating request presence from channel eligibility prevents a request from disappearing and gives the future scheduler a stable pending view.

## Scope

This is still a scheduler foundation, not a persistent asynchronous queue or a cycle-accurate multi-channel DMA engine. Only channel 2 has implemented device transfer routines; other channel endpoints, bus arbitration, transfer timing, and CPU stalls remain future work.

## Validation

GitHub Actions run 46033810262 completed successfully:
- CMake configure: passed.
- Host build: passed.
- CTest: 33/33 tests passed.

The first run exposed an outdated expectation: a request from a DPCR-disabled channel is now correctly present in the raw pending mask. The regression assertions were updated to reflect the separation between request presence and arbitration eligibility; the final run passed.
