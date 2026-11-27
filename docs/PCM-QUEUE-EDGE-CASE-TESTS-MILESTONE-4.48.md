# PCM queue edge-case validation milestone 4.48

## Change

Expanded the host-side PCM queue test to cover invalid pointer combinations and the bounded-queue overflow contract.

The test now verifies that:
- a null source is rejected for a non-zero write;
- a null destination is rejected for a non-zero read;
- null queue handles are rejected safely;
- zero-frame reads remain valid no-ops;
- a full queue rejects additional frames and increments the dropped-frame counter;
- rejected writes do not overwrite samples that were already queued.

Existing coverage continues to exercise underrun zero-fill, stereo ordering, wraparound reads/writes, and reset behavior.

## Validation status

The test change is committed. GitHub Actions host build and CTest must complete successfully before this milestone is considered validated. These are host-side queue tests and do not replace the Dreamcast KallistiOS cross-build, Flycast execution, or physical AICA listening tests.
