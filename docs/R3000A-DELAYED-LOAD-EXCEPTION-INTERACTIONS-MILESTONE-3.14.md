# R3000A delayed-load, link-branch and exception interactions — milestone 3.14

## Coverage added

- CPU regression: `LW $ra` followed by `BGEZAL $ra` uses the pre-load negative `$ra` for the branch decision. The branch remains not taken, while the unconditional link write to `$ra` takes precedence over the pending load.
- Machine regression: a branch delay-slot `LW` is followed by an overflowing `ADDI` at the branch target. Exception entry must retire the pending load, report the target instruction as EPC, and leave Cause.BD clear because the fault is not in the delay slot.
- COP0 regression: `MFC0` now schedules its GPR result through the R3000A one-instruction delayed-load path. The following instruction sees the old destination value; the value becomes visible after that instruction.
- COP0 regression: `MTC0` immediately following `LW` reads the old GPR source value, while the pending load retires after the COP0 write.

## Validation

GitHub Actions successfully configured and built the host project, then passed all 12 CTest tests on commit `6a7cbf489456863e7d73e9eec95103180dbc82e3`.

Run: https://github.com/oreldes62-star/DreamPSX/actions/runs/45769556077

The CI result validates the host-side CMake suite only. It does not establish a Dreamcast/KallistiOS build, BIOS boot, CDI image, or gameplay compatibility.

## Follow-up audit

Continue with delayed-load interactions for exception-causing instructions, COP0 register transfers, and the remaining R3000A instruction encodings. Each new regression should be followed by a fresh CI run before it is marked validated.
