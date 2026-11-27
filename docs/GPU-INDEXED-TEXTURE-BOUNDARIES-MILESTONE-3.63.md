# GPU indexed texture boundary milestone 3.63

## Added regression coverage

- 4-bit indexed sampling selects the expected packed nibble and resolves its CLUT entry.
- 8-bit indexed sampling selects the high byte from a packed VRAM word.
- An 8-bit CLUT lookup crossing the right edge of VRAM wraps to the start of the row.
- Palette index zero remains transparent and does not overwrite the destination pixel.

## Validation status

The tests are registered with CMake/CTest. They have been committed but have not yet been executed in this development session. A passing source-level test definition is not evidence of runtime correctness.

## Scope

This milestone strengthens host-side indexed-texture regression coverage. It does not establish full PlayStation GPU accuracy, Dreamcast performance, or game compatibility.
