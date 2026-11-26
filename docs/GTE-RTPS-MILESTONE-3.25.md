# GTE RTPS milestone 3.25

## Implemented

The GTE command dispatcher now recognizes RTPS (command 0x01) and performs a first complete single-vertex perspective transform:

- Rotation-matrix multiplication plus translation, with 44-bit MAC1–MAC3 overflow checks.
- IR1–IR3 saturation, including the RTPS-specific IR3 FLAG.22 check behavior.
- SZ3 depth extraction and four-stage SZ FIFO push.
- Hardware-style UNR reciprocal division using the documented 257-entry table-generation formula and two refinement steps.
- Near-plane/divide overflow reporting (FLAG.17).
- OFX/OFY projection, MAC0 overflow checks and signed screen-coordinate saturation (FLAG.14/13).
- SXY FIFO push.
- DQA/DQB depth-cue calculation into MAC0 and IR0 saturation (FLAG.12).
- FLAG summary-bit handling.

## Regression coverage

The host GTE test target now includes:
- Identity transform with screen center offset.
- Perspective projection of an offset vertex.
- Near-plane divide overflow and SX saturation.
- sf=0 MAC3/IR3/SZ3 behavior.
- Negative camera-space depth clamping to SZ3=0.

## Validation status

The host build and CTest suite passed on commit `bb4dbc85c16318774dbb71f1daaf340463a05c41`: 13/13 test targets passed, including the new sf=0 and negative-depth cases.

This is host-side validation only. RTPS accuracy has not yet been compared exhaustively against physical PS1 hardware. No Dreamcast/KallistiOS build, BIOS boot, CDI image, Flycast gameplay or GDEMU test is claimed.

## Next work

- Verify negative MAC0 arithmetic and RTPS depth-cue edge cases against independent test vectors.
- Add RTPT (triple perspective transform), reusing the single-vertex pipeline while preserving FIFO order and accumulated flags.
- Continue toward AVSZ3/AVSZ4 and the remaining GTE command families.
