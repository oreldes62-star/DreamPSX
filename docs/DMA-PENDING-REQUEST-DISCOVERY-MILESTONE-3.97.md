# DMA pending request discovery — milestone 3.97

## Delivered

- Added `psx_dma_pending_mask()` to derive pending channels from the controller state.
- A channel must be enabled in DPCR and have CHCR START/busy set.
- Manual sync mode additionally requires CHCR TRIGGER.
- Sync mode 3 is excluded as reserved/unsupported.
- Added tests covering enabled request and linked-list modes, manual mode without/with trigger, a disabled channel, reserved sync mode, null input, and selection from the discovered mask.

## Validation

GitHub Actions run 46033184643 completed successfully:
- CMake configure: passed.
- Host build: passed.
- CTest: 33/33 tests passed.

## Scope

This discovers pending channels and combines with the priority selector; it does not yet execute or queue multiple channel transfers. Only channel 2 currently has an implemented transfer endpoint. Arbitration timing, bus ownership, CPU stalls, and device-specific handshakes remain future work.
