# CD-ROM DMA channel 3 — milestone 4.11

## Delivered

- Added a 32-bit little-endian read operation for the CD-ROM data FIFO. It requires the data-request gate and four bytes of available sector data.
- Implemented DMA channel 3 as a CD-ROM-to-RAM burst endpoint.
- Enforced the currently modeled hardware conditions: device-to-RAM direction, SyncMode 0, CHCR start and forced-start bits, and DPCR channel enable.
- Routed channel 3 through the shared DMA priority selector and single-active-channel ownership model.
- Reused common completion bookkeeping so CHCR start clears, DICR channel-3 completion latches, and the DMA master IRQ path can observe the completion.
- Added a machine-bus integration test that transfers a complete 2048-byte ISO sector (512 words) into RAM and checks byte order, FIFO drain, CHCR completion, DICR flag, and transfer telemetry.

## Hardware reference

The PSX-SPX DMA channel reference identifies channel 3 as CD-ROM-to-RAM, documents SyncMode 0 block-count semantics, and notes that CD-ROM does not assert DMA3 DREQ; software therefore uses forced-start bit 28. It also documents the fixed CD-ROM DMA data-register bus address.

Reference: https://psx-spx.consoledev.net/dmachannels/

## Boundaries still outstanding

- This endpoint is synchronous and transfers only data already present in the emulated sector FIFO.
- No CD-sector scheduler, transfer-rate timing, DMA chopping, CPU bus stalls, or FIFO wait-state model is implemented.
- No 2352-byte raw-sector mode, subchannel, XA audio/data decoding, CDDA, or file-backed CUE/BIN/CDI loader is included.
- ISO image memory remains non-owning and must stay valid while mounted.
- Host CI does not validate Dreamcast SH-4 compilation, CDI packaging, Flycast execution, GDEMU behavior, or game compatibility.
