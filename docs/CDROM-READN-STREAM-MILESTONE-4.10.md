# CD-ROM continuous ReadN streaming — milestone 4.10

## Delivered

- ReadN now starts a continuous sector stream rather than stopping after one sector.
- Added a sector advancement entry point that refuses to overwrite an unread sector.
- Each advancement increments the BCD MSF address and loads the next 2048-byte ISO user-data sector.
- Data-ready INT1 is raised for each newly delivered sector; the command-complete INT3 remains independently acknowledgeable.
- End-of-image is reported through INT5 and stops the active stream.
- Added a machine-level sector-event entry point so a future timing scheduler can drive CD-ROM events and refresh CPU IRQ2 routing.
- Added tests for two distinct sequential sectors, full-sector draining, backpressure (no replacement of unread data), end-of-image, and machine-level IRQ2 propagation.

## Timing contract and limitations

The current API deliberately separates sector delivery from wall-clock timing. The future Dreamcast scheduler must call `psx_machine_cdrom_advance_sector()` at the correct emulated sector cadence after the previous sector has been consumed. This milestone does not yet model spindle/seek latency, rotational timing, continuous DMA channel 3 transfers, raw 2352-byte sectors, subchannel data, XA/CDDA audio, or a file-backed CDI/CUE loader. The mounted ISO buffer remains non-owning and must stay valid for the duration of the mount.

## Validation

GitHub Actions run `46049531700`: host configure and build succeeded; CTest passed all 34/34 tests. This is host-side validation only, not a Dreamcast build or game-compatibility result.
