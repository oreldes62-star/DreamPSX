# SIO0 cycle-driven controller transfer milestone

## Scope

The SIO0 digital-pad path now schedules each response in PS1 master-clock cycles instead of making received bytes visible immediately.

- The baud register is used as the base serial bit period.
- MODE bits 0–1 select a divider of 1, 16, 64, or 256.
- The current byte frame uses ten bit-times (start, eight data, stop).
- TX remains busy until the scheduled transfer completes.
- RX-ready and the response byte become visible only at completion.
- TX, RX-threshold, and controller-acknowledge interrupt requests are raised on completion.
- `psx_machine_advance_system_cycles()` advances the SIO transfer alongside timers, CD-ROM, and SPU scheduling.

The timing model is an initial deterministic approximation for the emulator scheduler. Exact electrical timing, parity/character-length modes, serial shift-register behavior, and measured retail-console timing remain future compatibility work.

## Tests

GitHub Actions host build: **48/48 tests passed** on commit `5dceb73e4f3c158c5a83afa11e974547de54ae99`.

The SIO test verifies:
- no RX-ready indication before the full byte duration,
- IRQ7 is raised when the acknowledged controller byte completes,
- the digital pad ID and two active-low button bytes,
- clearing SIO IRQ state and controller reset,
- disconnected-controller response handling.

The Dreamcast KallistiOS build is tracked separately in GitHub Actions. A successful host build does not prove execution on Flycast or physical Dreamcast hardware.
