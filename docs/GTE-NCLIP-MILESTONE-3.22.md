# GTE NCLIP command — milestone 3.22

## Scope

Implemented the first executable PlayStation GTE geometry command.

- Added a GTE command execution entry point.
- Implemented NCLIP (command code 0x06), using signed 16-bit SX/SY values from SXY0, SXY1 and SXY2.
- Computes the signed screen-space triangle area into MAC0 with a wide intermediate.
- Reports positive and negative MAC0 overflow in FLAG bits 16 and 15 and derives the summary bit.
- Connected COP2 command encodings in the R3000A execution path to the GTE command dispatcher.
- Retained the Status.CU2 availability check before command execution.

## Tests and validation

GTE unit tests cover counter-clockwise, clockwise and degenerate triangles, plus MAC0 overflow reporting. A CPU/machine integration test executes the encoded COP2 NCLIP instruction and checks MAC0.

GitHub Actions host CI passed on commit `dee84400f012bdfb2c38fbddd056f94c0b089822`: the host CMake build succeeded and all 13 CTest targets passed.

Run: https://github.com/oreldes62-star/DreamPSX/actions/runs/45891988284

## Deliberate limitations

NCLIP is the first supported GTE command, not a complete GTE. MVMVA, RTPS, lighting, color/depth cueing, command timing/latency and the remaining command set are still outstanding. The validation is host-side only; it does not establish a Dreamcast/KallistiOS build, BIOS boot, CDI image or gameplay.
