# GTE MVMVA selector validation — milestone 3.24

## Scope

This milestone expands the host-side regression coverage for the MVMVA command without changing its arithmetic implementation.

## Coverage added

- Vector selector V1, using the second input-vector register pair.
- Vector selector V2, using the third input-vector register pair.
- Vector selector IR, using IR1–IR3 as the source vector.
- Light-matrix selector with an identity matrix.
- Color-matrix selector with an identity matrix.
- Existing RT-matrix, translation, far-color quirk, shift, clamp and overflow cases remain in the same GTE test target.

The selector cases deliberately use identity matrices and simple integer vectors so that incorrect instruction-field decoding or register-bank selection is easy to detect.

## Validation

GitHub Actions host build and CTest completed successfully on commit `aad60bf41ee98c64190932491c382e2b73e3e191`:

- Build: successful.
- CTest: 13/13 test targets passed.
- `test_gte`: passed with the new selector regression cases.

This is host-side validation only. It does not establish Dreamcast/KallistiOS buildability, hardware timing accuracy, BIOS boot, CDI generation or game compatibility.

## Next step

Continue validating the special/reserved MVMVA matrix selector against independent hardware references, then implement RTPS as a separate staged GTE command with fixed-point perspective division and FIFO behavior.
