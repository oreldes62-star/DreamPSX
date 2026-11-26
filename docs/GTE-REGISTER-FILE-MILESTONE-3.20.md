# GTE register-file foundation — milestone 3.20

## Scope

Introduced the first standalone Geometry Transformation Engine (GTE) state module and attached it to the machine state.

- Separate 32-entry data and control register banks.
- Deterministic reset, including machine reset.
- Signed 16-bit read behavior for IR0–IR3.
- SXYP data-register alias with SXY FIFO push behavior.
- IRGB writes unpack 5-bit channels into IR1–IR3; IRGB/ORGB reads expose packed clamped channels.
- LZCS writes calculate the read-only LZCR leading-sign count.
- FLAG control-register writes are masked and the summary bit is derived.
- Bounds and null-pointer checks on register access.

## Validation

GitHub Actions host CI passed on commit `bef22c70b9aa6b259924c405a216306db72a395c`: the build succeeded and all 13 CTest targets passed.

## Deliberate limitations

This milestone is only the GTE register-file foundation. It does not yet implement GTE command execution, fixed-point matrix/vector transformations, perspective projection, lighting, depth cueing, or COP2 instruction integration. It is not a Dreamcast/KallistiOS build, BIOS boot, CDI image, or gameplay test.
