# DPCR decoding foundation — milestone 3.95

## Delivered

- Added public helpers to query whether a DMA channel is enabled by its DPCR field.
- Added a public helper to read a channel's three-bit DPCR priority value.
- Invalid channel indices and uninitialized controllers are handled safely.
- GPU channel 2 manual, request/block, and linked-list transfer entry points now use the shared channel-enable decoder rather than duplicating raw DPCR bit arithmetic.
- Added register-level tests confirming channel 2 priority remains 3 while its enable bit is independently toggled, and invalid channel indices are rejected safely.

## Validation

GitHub Actions run 46032689517 completed successfully:
- CMake configure: passed.
- Host build: passed.
- CTest: 33/33 tests passed.

## Scope and next work

This is the decoding foundation for a future multi-channel DMA scheduler. It does not yet arbitrate simultaneous channel requests or model priority-dependent bus ownership, transfer durations, CPU stalls, or GPU FIFO backpressure. Only the GPU channel's transfer engine is currently implemented.
