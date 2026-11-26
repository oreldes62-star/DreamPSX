# GTE GPL milestone 3.31

## Implemented

The GTE dispatcher now supports GPL (command 0x3E), accumulated general-purpose interpolation:

- For each color/vector channel, the previous signed MAC value is multiplied by 4096.
- The product IR0 × IR1/IR2/IR3 is added to that accumulator.
- sf selects whether the combined result is shifted by 12 bits.
- MAC1–MAC3 are updated and checked for 44-bit overflow.
- IR1–IR3 are updated with lm-controlled saturation.
- The FLAG summary bit is updated when an arithmetic or saturation flag is raised.

## Regression coverage

The host GTE tests check:
- Positive and negative channel accumulation.
- sf=1 output after fixed-point shifting.
- sf=0 raw accumulated MAC values.

## Validation

GitHub Actions host build and CTest passed on commit `e5d0b8db2f9556c2d5aa5b45b53d22732ec6890e`:

- Build successful.
- 13/13 test targets passed.
- `test_gte` passed with the new GPL cases.

This validates the host implementation only. No Dreamcast/KallistiOS build, PS1 BIOS boot, CDI generation, Flycast gameplay or GDEMU compatibility is claimed.

## Next work

Continue with the GTE color pipeline and compare arithmetic against independent PS1 reference vectors before treating the implementation as hardware-accurate.
