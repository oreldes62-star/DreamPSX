# GPU texture and VRAM boundary validation milestone 3.58

## Implemented

- Added `tests/test_gpu_texture_wrap.c` as a dedicated host regression target.
- Direct 15-bit texture sampling is tested across the right VRAM edge: page X 960 plus U 255 resolves to X 191.
- 8-bit indexed texture fetch is tested across the packed-texel VRAM edge.
- The associated CLUT lookup is independently tested across the horizontal VRAM edge.
- GP0(A0h) image upload is tested at origin (1023, 511), confirming horizontal and vertical wrap for a 2x2 transfer.
- Registered the test in the host CMake/CTest suite.

## Important hardware-coordinate distinction

The PS1 textured-triangle command supplies 8-bit U and V coordinates. Texture page Y is selected at 0 or 256, so page Y plus V spans 0..511 and cannot itself exceed the 512-line VRAM height. Therefore this milestone does not claim a separate vertical wrap for ordinary texture sampling. Vertical wrap is exercised through the VRAM image-transfer path.

## Validation status

The test source and CMake registration are committed. A successful compiler/CTest or GitHub Actions result is still required before calling the milestone fully validated.

## Remaining limitations

This is host-side regression coverage, not proof of hardware-accurate behavior. More reference-based checks are needed for texture-page boundaries, texture-window interactions, indexed 4-bit edge cases, CLUT layout and rendering fidelity. No Dreamcast/KallistiOS binary or CDI image is produced by this work.
