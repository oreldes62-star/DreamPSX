# DMA master interrupt delivery — milestone 3.90

## Delivered

- Added `psx_dma_irq_output()` to expose the DMA controller's DICR master interrupt condition.
- The master output is asserted by DICR force bit 15, or by master enable bit 23 combined with at least one channel whose enable and completion flag are both set.
- The CPU bus now synchronizes this output with interrupt-controller source 3 after DMA register writes and channel execution.
- Clearing the DICR completion flag deasserts the DMA source line. The already latched I_STAT request remains pending until software acknowledges it, matching the interrupt controller's edge-latched behavior.
- Other interrupt sources are not modified.

## Validation added

The machine-bus test enables the DMA master interrupt and channel 2, starts a manual GPU transfer, and checks that I_STAT source 3 becomes pending and the DMA source line is asserted. It then clears the DICR channel flag, verifies that the source line drops while I_STAT remains latched, and acknowledges I_STAT.

## Limitations

- DMA transfers are still executed synchronously; interrupt timing is therefore immediate at the end of the modeled transfer.
- DMA channel completion flags and master IRQ behavior are implemented, but detailed per-channel transfer timing, bus arbitration and hardware-accurate interrupt latency remain future work.
- The test source has been added but has not been compiled or run in this environment.
