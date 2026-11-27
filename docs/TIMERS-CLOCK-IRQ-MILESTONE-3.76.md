# Timer clock selection and IRQ events — milestone 3.76

Adds explicit clock-domain selection to the root-counter foundation:
Timer 0 supports system/dot clock selection, Timer 1 system/HBlank, and
Timer 2 system/system-divided-by-eight. A clock advance is ignored when its
domain does not match the programmed selection.

Target and overflow events can request timer IRQ sources. The API reports
which timer generated an IRQ event; the machine adapter routes Timer 0/1/2
to interrupt controller sources 4/5/6 using a latched pulse. One-shot versus
repeat behavior is represented by MODE bit 6.

This milestone does not yet synthesize dot-clock or HBlank edges from GPU
timing, model timer synchronization modes, implement MODE bit 7 IRQ toggling,
or integrate a CPU-cycle scheduler. The frontend/machine timing loop must
supply clock ticks explicitly. Timer 2's divided clock is exposed as a domain;
the divider phase accumulator is future work.

Tests are added for source gating and one-shot IRQ requests. No build or test
execution is claimed unless a CI run is reported separately.
