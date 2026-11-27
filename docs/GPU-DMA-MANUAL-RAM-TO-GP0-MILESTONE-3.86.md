# First GPU DMA transfer: manual RAM-to-GP0 — milestone 3.86

## Objective

Move the first real payload through the PS1 DMA path instead of only exposing its registers. This increment supports channel 2 (GPU), manual synchronization mode, RAM-to-device direction, and GP0 writes.

## Behavior

- A write to channel 2 CHCR starts the path when direction bit 0 is set, synchronization mode is manual (bits 9–10 = 0), enable bit 24 and trigger bit 28 are both set, and DPCR channel-2 enable bit 11 is set.
- The transfer count is BCR low 16 bits; zero represents 65,536 words in manual mode.
- Each 32-bit word is read from emulated RAM through `psx_memory_read32` and submitted to the GPU command/data port through `psx_gpu_gp0_write`.
- MADR advances or decrements by four bytes according to CHCR step bit 1 and wraps in the 24-bit physical DMA address space.
- On successful completion, the manual enable/trigger bits are cleared, the low BCR count is cleared, and DICR channel-2 completion flag (bit 26) is set.

## Validation added

`tests/test_machine_bus.c` programs a three-word GP0 fill-rectangle packet in RAM, enables channel 2 in DPCR, starts a manual DMA transfer, then verifies the resulting VRAM pixel, final MADR, cleared CHCR start bits and DICR channel flag.

## Scope and limitations

This is a synchronous first slice, not a complete DMA engine. It does not yet support GPU request/block mode, GPU linked-list mode, other DMA channels, arbitration/priority scheduling, chopping, cycle timing, or raising the system DMA IRQ line. GPU-to-RAM readback is added separately in milestone 3.87. A RAM read or GP0 write failure aborts the function and leaves the channel state available for diagnosis; rollback of already-submitted words is not attempted. The implementation is intentionally explicit about these limits rather than claiming general DMA compatibility.

Tests have been added but have not been compiled or run in this environment.
