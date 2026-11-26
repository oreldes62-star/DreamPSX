# R3000A delayed-load and control-flow interactions — milestone 3.13

## Coverage added

- CPU regression: a BEQ immediately following an LW compares the pre-load GPR value, then the pending load retires after the branch has read its operands. The test verifies the branch decision, updated register value, delay-slot instruction and target instruction.
- Machine regression: a hardware IRQ asserted after a branch but before its delay slot is deferred until the delay-slot LW executes. At the next instruction boundary, the pending load retires before interrupt entry; EPC points to the branch target and Cause.BD remains clear.

## Validation

GitHub Actions configured CMake, built the core and test executables, and passed all 12 CTest tests on commit `0666f71390e686b91d0082b2e72e0aacebac70c5`.

Run: https://github.com/oreldes62-star/DreamPSX/actions/runs/45768948246

This confirms host-side behavior covered by the current tests. It does not yet validate a Dreamcast/KallistiOS build, BIOS boot, CDI image, or gameplay.

## Next step

Continue the CPU audit with load-delay interactions involving link-register writes, exceptions in delay slots, and the instruction pipeline around branch targets. Preserve architectural behavior with focused regression tests before adding more subsystems.
