# DPCR priority selector — milestone 3.96

## Delivered

- Added `psx_dma_select_channel()` to select among a caller-supplied pending-channel bitmask.
- Disabled channels are ignored.
- Lower DPCR priority values win (0 is highest).
- Equal priorities are resolved deterministically by lower channel index.
- Invalid/uninitialized controllers and empty or non-selectable masks return -1.
- Added tests for priority ordering, disabled channels, tie-breaking, and empty/non-selectable requests.

## Validation

GitHub Actions run 46032947383 completed successfully:
- CMake configure: passed.
- Host build: passed.
- CTest: 33/33 tests passed.

## Scope

This is a deterministic arbitration primitive, not yet a cycle-accurate multi-channel DMA engine. It does not queue hardware requests, execute transfers from multiple endpoints, model bus ownership, CPU stalls, or per-channel transfer timing. The GPU channel remains the only implemented DMA transfer endpoint.
