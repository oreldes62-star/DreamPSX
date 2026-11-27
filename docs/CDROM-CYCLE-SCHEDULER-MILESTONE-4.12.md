# CD-ROM cycle-driven sector scheduler — milestone 4.12

## Delivered

- Connected continuous ReadN sector delivery to the shared PS1 system-cycle clock.
- Modeled the currently selected drive speed as double speed: 150 sectors per second.
- Derived the sector interval from the 33,868,800 Hz PS1 system clock: 225,792 cycles per sector.
- Kept a persistent cycle phase so splitting elapsed time across calls does not change the sector deadline.
- Routed each timed sector event through the machine-level CD-ROM IRQ2 refresh path.
- Added deterministic tests for a deadline split across calls, the exact boundary, sequential sector delivery, and a delayed consumer.

## Timing references

PSX-SPX documents ReadN/ReadS delivery at 75 sectors/s in normal mode and 150 sectors/s at double speed. Sony's PlayStation developer guide also describes the dual-speed drive and its 150 KB/s normal data rate. This milestone selects the common double-speed mode.

- https://psx-spx.consoledev.net/cdromdrive/
- https://psx.arthus.net/sdk/Psy-Q/DOCS/TECHNOTE/scee_dev.pdf

## Simplifications and next work

- Drive speed is currently fixed at double speed; the CD-ROM mode register does not yet select normal/double speed.
- Seek latency, command response latency, retry behavior, rotational phase, and pause/stop command timing are not modeled.
- The controller has one exposed 2048-byte sector buffer. If the consumer misses a deadline, this implementation retains one overdue event and waits for the current sector to drain. It does not yet emulate the hardware's internal buffering or sector-loss/overrun behavior.
- CPU instructions supply the system cycles; a future finer scheduler must integrate DMA bus occupancy and device event ordering at sub-instruction granularity.
- Host CI does not validate Dreamcast SH-4 compilation, CDI packaging, Flycast execution, GDEMU behavior, or game compatibility.
