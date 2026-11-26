# GTE RTPT milestone 3.26

## Implemented

The GTE command dispatcher now recognizes RTPT (command 0x30). It reuses the RTPS single-vertex pipeline for V0, V1 and V2 in sequence:

- Three rotation/translation and perspective transformations.
- Correct use of V0, V1 and V2 input registers.
- Three successive pushes into the SXY FIFO, leaving the projected vertices in SXY0/SXY1/SXY2 order.
- Three successive pushes into the SZ FIFO, leaving the three depths in SZ1/SZ2/SZ3.
- Accumulation of FLAG bits across all three vertex operations rather than losing an earlier vertex's exception when a later vertex completes.

## Regression coverage

The host GTE test target checks a three-vertex identity-camera case:
- V0 projects to the screen center.
- V1 projects to the expected positive X offset.
- V2 projects to the expected positive Y offset.
- The three depth values remain in FIFO order.

## Validation

GitHub Actions completed the host build and all 13 CTest targets successfully on commit `cd90167b87236a00bec98bcf5af46325ebb4aac1`. The new RTPT checks passed in `test_gte`.

This is host-side validation only. It does not establish Dreamcast/KallistiOS buildability, physical PS1 hardware accuracy, BIOS boot, CDI generation, Flycast gameplay or GDEMU compatibility.

## Next work

Expand RTPT coverage for accumulated overflow flags and unusual depth values, then continue with AVSZ3/AVSZ4 and remaining GTE command families.
