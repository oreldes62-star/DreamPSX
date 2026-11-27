# GPU VRAM readback through manual DMA — milestone 3.87

## Objective

Complete the opposite direction of the first channel-2 transfer path: move GPUREAD image data into emulated RAM using manual DMA.

## Behavior

- Channel 2 manual mode now accepts CHCR direction bit 0 clear for device-to-RAM transfers.
- The GPU must be configured for GPUREAD-to-CPU DMA (GP1(04h) direction value 3) and have an active GP0(C0h) VRAM-to-CPU image transfer.
- Each 32-bit GPUREAD value is written to the destination address in RAM.
- MADR advances or decrements by four bytes according to CHCR step bit 1.
- The BCR low halfword supplies the number of 32-bit words. A zero value retains the manual-mode interpretation of 65,536 words.
- Completion clears CHCR start/trigger bits and sets DICR channel-2 completion flag bit 26.
- The engine stops with failure if the GPU readback payload is not active or is exhausted before the programmed DMA word count, rather than fabricating zero-filled words from an idle GPUREAD port.

## Validation added

`tests/test_machine_bus.c` now performs both directions through the machine bus:

1. RAM-to-GP0 sends a fill-rectangle command and checks VRAM.
2. GP0(C0h) selects two pixels from VRAM, and manual DMA writes the packed two-pixel GPUREAD word into RAM. The test checks the destination word, final MADR, CHCR completion bits and DICR flag.

## Limitations

This remains a synchronous channel-2 manual-mode implementation. GPU request/block mode, linked-list mode, DMA channels 0, 1, 3–6, bus arbitration, cycle-accurate scheduling, chopping, and delivery of the DMA interrupt line are not implemented. Tests are added but have not been compiled or run in this environment.
