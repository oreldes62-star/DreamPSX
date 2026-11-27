# GPU DMA linked-list mode — milestone 3.88

## Delivered

- Channel 2 accepts CHCR sync mode 2 for RAM-to-GP0 linked-list transfers.
- Each packet header is read from RAM; its high byte is the number of following GP0 words and its low 24 bits point to the next packet.
- Packet headers are not submitted to GP0. Payload words are streamed in order, allowing a GP0 command and its parameters to cross packet boundaries.
- The 24-bit pointer `0x00ffffff` terminates the list.
- DPCR channel-2 enable and GP1 DMA direction 2 are required.
- Linked-list mode does not require the manual-mode trigger bit or use BCR as a word count.
- On successful completion, MADR reports the end marker, CHCR busy clears, and DICR channel-2 completion flag is set.
- Internal safety guards cap traversal at 65,536 packet headers and 1,048,576 payload words. These are defensive implementation limits, not claims about hardware limits.

## Validation added

The machine-bus test constructs a two-packet list in RAM. The first packet contains the fill command and XY parameter; the second contains WH and the end marker. It checks the resulting VRAM pixel, MADR, CHCR busy state and DICR completion flag.

## Limitations

This is a synchronous first implementation. DMA arbitration, cycle timing, request/block mode, GPU FIFO backpressure, interrupt-controller delivery of DICR master IRQ, exact error/abort semantics and hardware-level conformance remain incomplete. The test has been added but has not been compiled or executed in this environment.
