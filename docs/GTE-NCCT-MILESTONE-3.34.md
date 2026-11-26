# GTE NCCT milestone 3.34

## Implemented

Added NCCT (Normal Color Color Triple, command 0x3F).

- Executes the NCCS color/lighting pipeline for V0, V1 and V2.
- Preserves the source V0 registers after the command.
- Pushes one RGB result per vertex, preserving FIFO order.
- Accumulates FLAG bits across all three vertex operations and recomputes the summary bit.

## Regression coverage

The host GTE test uses identity light and color matrices and three distinct normals. It checks the three expected RGB FIFO entries and verifies that V0 source registers are restored.

## Validation scope

This milestone is subject to GitHub Actions host build and test validation. It does not establish a Dreamcast/KallistiOS build, PS1 BIOS boot, CDI image, Flycast gameplay or GDEMU compatibility.
