# CD-ROM Setmode, Pause and Stop — milestone 4.13

## Delivered

- Implemented command 0Eh Setmode with its one-byte mode parameter.
- Mode bit 7 selects normal (75 sectors/s) or double (150 sectors/s) sector cadence.
- Reset and Init restore normal-speed mode.
- Added asynchronous Pause (09h) and Stop (08h): each produces its immediate INT3 acknowledgement, then a delayed INT2 completion.
- Pause stops sector delivery while keeping the spindle-on status; Stop stops delivery, clears the data FIFO/request gate, and clears spindle-on status.
- Added a shared PS1-cycle countdown for command completion and routed the resulting CD-ROM interrupt level through machine IRQ2.
- ReadN now reflects the spindle-on and reading status bits.
- Tests cover normal/double speed selection, exact before/at completion boundaries, Pause/Stop status changes, and machine-level IRQ2 propagation.

## Timing basis

The delay constants use the measured average command-response timings published by PSX-SPX for a PAL PSone, expressed in 33.8688 MHz system cycles. These are representative measured values, not a universal exact timing guarantee across all console revisions.

Reference: https://psx-spx.consoledev.net/psx-spx.pdf (CDROM response timings); command and mode definitions: https://github.com/psx-spx/psx-spx.github.io/blob/master/docs/cdromdrive.md

## Validation

GitHub Actions host build and CTest passed on commit `0b578c25c67fc6491764d6c7f0ff4856348ea219` (34/34 tests).

## Limitations

- Setmode currently models only speed bit 7; XA, sector-size, filter, report, autopause and CD-DA mode bits are not implemented.
- Pause/Stop use fixed measured-average delays, not spindle/seek state-dependent ranges.
- Command queueing, response FIFO overflow behavior, exact command rejection timing and concurrent command arbitration remain simplified.
- No physical drive, CUE/BIN/CDI file loader, XA/CDDA decoding, subchannel, or game compatibility validation is provided.
- Host CI does not validate the Dreamcast SH-4 target, CDI packaging, Flycast execution, GDEMU behavior, or real-time performance.
