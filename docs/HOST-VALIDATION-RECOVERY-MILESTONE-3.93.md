# Host validation recovery — milestone 3.93

## Delivered

The first complete host CI run exposed five failing tests. The failures were investigated and corrected:

- DMA register-map test now recognizes DPCR at offset 0x70 and verifies DICR control writes replace writable control bits.
- CPU/GTE integration tests now wait for the documented NCLIP execution latency before inspecting results.
- Timer tests now respect the timer MODE register's read-to-clear event flags.
- Timer IRQ integration enables the relevant I_MASK sources, including after machine reset.
- GPU DMA fill test uses non-overlapping 16-pixel-aligned fill rectangles, matching GP0 fill-rectangle behavior.

## Validation

GitHub Actions run 46032208551 completed successfully:
- CMake configure: passed.
- Full host build: passed.
- CTest: 33/33 tests passed.

This validates the host build and registered test suite only. It does not yet validate the Dreamcast toolchain, CDI generation, hardware timing, or commercial game compatibility.
