# GPU DMA request/block mode — milestone 3.89

## Delivered

- Channel 2 now dispatches CHCR synchronization mode 1 to a dedicated request/block transfer path.
- BCR low 16 bits specify words per block; high 16 bits specify block count. The transfer length is their product.
- A zero block-size or block-count field is interpreted as 65,536, with a defensive total-word cap of 1,048,576.
- RAM-to-GP0 and GPUREAD-to-RAM directions are supported, using GP1(04h) direction 2 and 3 respectively.
- MADR follows CHCR step direction, BCR clears on completion, CHCR busy clears, and DICR channel-2 completion flag is raised.
- Unlike manual mode, request mode does not require the manual trigger bit.

## Validation added

Machine-bus tests cover two RAM-to-GP0 blocks containing two separate fill commands and a GPUREAD-to-RAM block transfer. They check VRAM/RAM data, MADR, BCR, CHCR busy and the DMA completion flag.

## Limitations

This first implementation executes synchronously and does not yet model per-block GPU request stalls, FIFO depth/backpressure, arbitration or DMA cycle timing. The GPU's request bit remains an approximation. DMA master interrupt delivery into the interrupt controller is also not yet connected. Tests are added but have not been compiled or run in this environment.
