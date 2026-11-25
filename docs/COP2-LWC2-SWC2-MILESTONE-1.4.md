# DreamPSX COP2 milestone 1.4 — LWC2 / SWC2

## Implemented

- `LWC2` (opcode `0x32`) loads one aligned word from the CPU memory bus into a GTE data register.
- `SWC2` (opcode `0x3A`) stores one GTE data register to the CPU memory bus.
- Both require `Status.CU2` (bit 30); disabled access raises Coprocessor Unusable (ExcCode 11, CE=2).
- Unaligned addresses raise Address Error Load (ExcCode 4) or Address Error Store (ExcCode 5), and update `BadVAddr`.

## Validation

`make clean && make test` passes with the host compiler flags in the Makefile. CPU tests cover successful LWC2/SWC2 round-trip transfers, misaligned addresses, and CU2-disabled access.

## Limitations

- The bus callbacks cannot report read/write faults.
- Transfers are synchronous; instruction timing and GTE stalls are not modelled.
- No Dreamcast/KallistiOS target build or CDI image exists yet.
- The implementation has not been validated on physical PlayStation hardware.
