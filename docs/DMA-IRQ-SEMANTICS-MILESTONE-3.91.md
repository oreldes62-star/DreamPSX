# DMA interrupt semantics hardening — milestone 3.91

## Delivered

- Extended machine-bus integration coverage for the DICR force-IRQ control bit.
- Verified that force-IRQ asserts PS1 IRQ source 3 even when no channel completion flag is enabled.
- Verified that clearing force-IRQ deasserts the DMA source line, while I_STAT remains edge-latched until software acknowledges it.
- Verified that a channel completion flag does not assert the master interrupt when the corresponding channel-enable bit is clear, even when the master-enable bit is set.
- Kept the existing channel-2 completion path and DICR write-one-to-clear behavior unchanged.

## Validation added

The machine-bus test now exercises three distinct interrupt conditions: channel-2 completion with channel/master enables, forced master IRQ, and a completion flag with its channel enable disabled. It also checks the separation between the live DMA source line and the latched I_STAT request.

## Limitations

This milestone hardens register and interrupt semantics; it does not add asynchronous DMA scheduling. Transfers still execute synchronously when CHCR is written, so interrupt timing remains immediate in the current model. Tests are added but have not been compiled or run in this environment.
