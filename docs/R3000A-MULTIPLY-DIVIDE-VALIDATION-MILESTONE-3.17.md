# R3000A multiply/divide edge-case validation — milestone 3.17

## Scope

Expanded the host regression coverage for the R3000A integer multiply/divide unit without changing the arithmetic implementation.

Coverage includes:
- Signed division with a negative dividend, checking quotient truncation toward zero and remainder sign.
- The `INT32_MIN / -1` corner case, ensuring the implementation avoids C signed-division overflow.
- Unsigned division and multiplication, checking both HI and LO result words.
- Signed multiplication with a negative operand.
- Signed and unsigned division-by-zero behavior, including the signed quotient's dependence on the dividend sign.

## Validation

GitHub Actions host CI passed on commit `8de68e787779da23c0eeaa36d6daf1cb39014342`.

The checks validate the host-side CMake/CTest suite only. They do not establish a Dreamcast/KallistiOS build or runtime compatibility.
