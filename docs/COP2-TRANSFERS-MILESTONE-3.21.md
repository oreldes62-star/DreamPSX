# COP2 transfers and GTE memory access — milestone 3.21

## Scope

Connected the R3000A execution path to the machine's GTE register state.

- MTC2 writes a CPU GPR value to a GTE data register.
- MFC2 reads a GTE data register into a CPU GPR using the R3000A one-instruction delayed-load behavior.
- CTC2 writes a GPR value to a GTE control register.
- CFC2 reads a GTE control register using the same delayed-load behavior.
- LWC2 and SWC2 transfer GTE data registers through the main memory bus.
- COP2 access checks Status.CU2 and reports Coprocessor Unusable when unavailable.
- The existing CPU stepping API remains available; the machine-aware path passes its GTE state into instruction execution.

## Validation

GitHub Actions host CI passed on commit `2b5cd105f6421442508171b35dcd2bba61e0ff97`. The CMake build succeeded and all 13 CTest targets passed.

Run: https://github.com/oreldes62-star/DreamPSX/actions/runs/45891484089

## Deliberate limitations

COP2 command execution (MVMVA, RTPS, NCLIP, lighting, color/depth cueing and related operations) is not implemented yet. Encoded GTE commands currently return Reserved Instruction. This milestone is host-side CPU/GTE integration only; it is not a Dreamcast/KallistiOS build, BIOS boot, CDI image, or gameplay test.
