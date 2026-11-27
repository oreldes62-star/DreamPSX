# CD-ROM controller foundation — milestone 4.08

## Delivered

- Added a dedicated `PsxCdrom` device and attached it to `PsxMachine`.
- Mapped the four byte-wide CD-ROM registers at `0x1F801800–0x1F801803` through the CPU bus.
- Implemented the bank selector and the initial bank-0 / bank-1 register behavior.
- Added bounded 16-byte parameter and response FIFOs with FIFO status bits.
- Implemented initial commands: Getstat, Setloc and Init.
- Unsupported commands return an explicit error response and INT5 rather than fabricated sector data.
- Added CD-ROM interrupt enable/flag handling and routed the enabled device interrupt to interrupt-controller source 2 (IRQ2).
- Added standalone controller tests and machine-bus/IRQ integration coverage.

## Deliberate limitations

This is the controller foundation, not a playable disc subsystem yet. There is no ISO/CUE image backend, sector seek/read engine, 2048/2352-byte sector delivery, data FIFO, XA audio, CDDA, subchannel decoding, or rotational timing. ReadN and other not-yet-implemented commands report an error instead of pretending that a disc was read. The data register currently returns zero until the sector pipeline is implemented.

## Validation

GitHub Actions host build and CTest validation pending for this commit series.
