# GTE MVMVA command milestone 3.23

## Delivered

DreamPSX now executes the PlayStation GTE MVMVA command (command code `0x12`) through the existing COP2 command path.

Implemented behavior:

- Decodes the matrix, vector, translation, fractional-shift (`sf`) and IR lower-bound (`lm`) selectors.
- Reads V0/V1/V2 or IR1–IR3 and the RT, light or color matrix register banks.
- Computes the 44-bit signed MAC accumulators and records positive/negative overflow flags at intermediate accumulation stages.
- Applies the optional arithmetic 12-bit fractional shift and saturates IR1–IR3 to the selected signed or non-negative range.
- Models the documented far-colour translation (FC) quirk: the first-column intermediate is discarded from the final MAC, while its overflow and IR saturation flags remain observable.
- Handles the special matrix selector 3 using the documented GTE garbage-matrix inputs.

## Regression coverage

The host test suite covers:

- Identity transform with translation.
- `sf=0` fixed-point output.
- `lm=1` negative IR saturation.
- Negative 44-bit MAC1 overflow and the corresponding FLAG bit.
- FC translation quirk and final MAC values.
- Existing NCLIP, GTE register-file and COP2 integration behavior.

## Validation

GitHub Actions host CI passed all 13 CTest targets on commit `e85bfa6de764dd03df32f593ba9ae24a95e4da33`.

This is host-side validation only. It does not validate a Dreamcast/KallistiOS build, BIOS boot, CDI image, Flycast execution or gameplay.

## Remaining accuracy work

MVMVA is an initial implementation, not a claim of cycle-accurate GTE emulation. More hardware-reference vectors are needed for every selector combination, especially the special matrix and FC corner cases, and for precise overflow/saturation timing. Other GTE commands (including RTPS/RTPT, lighting, depth cueing, color operations and FIFO side effects) remain future work.
