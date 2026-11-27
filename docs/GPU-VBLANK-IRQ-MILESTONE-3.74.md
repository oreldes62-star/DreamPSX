# GPU VBlank IRQ integration — milestone 3.74

## Scope

Connects the GPU scanline scheduler to the machine interrupt controller. The
machine API advances one scanline, drives PS1 interrupt source 0 (VBlank), and
refreshes the CPU's COP0 Cause.IP2 view.

## Behavior

- VBlank is IRQ source 0 in the PS1 interrupt controller.
- The source level follows the GPU's VBlank state and remains asserted for the
  complete blanking interval.
- The interrupt controller latches the rising edge in I_STAT; software clears
  the pending bit using the existing write-zero-to-clear semantics.
- Leaving VBlank deasserts the source line, allowing a later VBlank interval
  to create a fresh rising edge.
- The existing interrupt mask and COP0 global/mask bits continue to control
  whether the CPU may accept the request.

## API

`psx_machine_video_advance_scanline` advances the GPU by one scanline,
updates IRQ0, and refreshes Cause.IP2. The frontend remains responsible for
calling this API; the CPU instruction stepper is not yet cycle-scheduled.

## Validation

A new host test checks VBlank entry/exit, IRQ0 level, I_STAT edge latching,
write-zero-to-clear acknowledgement, Cause.IP2 propagation and re-arming on
the next VBlank. Host tests do not establish Dreamcast hardware timing.
