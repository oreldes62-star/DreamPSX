# DPCR-triggered GPU DMA launch — milestone 3.94

## Delivered

- GPU channel 2 is re-evaluated when CHCR is programmed and when DPCR is written.
- A transfer armed while channel 2 is disabled remains pending instead of being silently lost.
- Enabling the channel later launches the already-programmed manual, request/block, or linked-list transfer through the same dispatch path.
- DMA IRQ source 3 is refreshed after the re-evaluation, including a completion triggered by the DPCR write.

## Validation

GitHub Actions run 46032498601 completed successfully:
- CMake configure: passed.
- Host build: passed.
- CTest: 33/33 tests passed.

The machine-bus integration test now verifies that a manual GPU DMA remains busy and does not modify VRAM while DPCR disables channel 2, then completes and writes the expected pixel when the DPCR enable bit is set.

## Limitations

This remains a synchronous transfer engine. It does not yet model bus arbitration, DMA priority scheduling, transfer-time CPU stalls, GPU FIFO backpressure, or cycle-accurate DMA timing. It is host-tested, not yet validated on Dreamcast hardware or in a Dreamcast-targeted build.
