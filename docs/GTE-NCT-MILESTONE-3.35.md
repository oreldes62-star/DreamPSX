# GTE NCT milestone 3.35

## Implemented

Added NCT (Normal Color Triple, command 0x20).

- Runs the NCS lighting/color pipeline for V0, V1 and V2.
- Produces one unmodulated RGB result per vertex.
- Pushes all three results through the RGB FIFO in vertex order.
- Restores the source V0 registers and accumulates FLAG bits across the three operations.

## Regression coverage

The host GTE test uses identity light and color matrices with three distinct normals. It checks all three FIFO outputs and verifies that V0 source registers are preserved.

## Validation scope

GitHub Actions host build/test validation is pending. This does not establish a Dreamcast/KallistiOS build, PS1 BIOS boot, CDI image, Flycast gameplay or GDEMU compatibility.
