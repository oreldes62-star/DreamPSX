# Timer register foundation — milestone 3.75

## Scope

Adds the three PS1 root-counter register banks at 1F801100h, 1F801110h and
1F801120h. Each exposes a 16-bit counter, mode and target in four-byte slots.

## Implemented behavior

- Reset initializes all three counters and registers.
- Counter and target reads/writes preserve their 16-bit values.
- Writing MODE resets the counter and stores its writable low ten bits.
- Reading MODE returns the target/overflow event flags and clears those
  read-to-clear flags.
- A deterministic external tick API advances a selected counter, detects
  target matches and 16-bit overflow, and supports reset-at-target.

## Deliberate limitations

This is a register/tick foundation, not complete root-counter emulation.
Clock-source selection, synchronization modes, HBlank/dot-clock wiring,
CPU-cycle integration and timer IRQ generation are not implemented yet.
The frontend or a later machine scheduler must explicitly supply ticks.

## Validation

A host test covers register spacing, reset-at-target, read-to-clear event
flags, independent timer state, overflow and invalid accesses. The test is
added to CTest but has not been executed in this environment.
