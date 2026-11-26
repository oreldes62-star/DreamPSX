# GTE DPCT milestone 3.37

## Implemented

Added DPCT (Depth Cueing Triple, command 0x2A).

DPCT performs three DPCS-style interpolation operations. Each iteration consumes the current oldest RGB FIFO entry (RGB0), interpolates its R/G/B channels toward RFC/GFC/BFC using IR0, applies sf/lm behavior and MAC/IR limits, and pushes the result to the RGB FIFO. The CODE byte is taken from RGBC for each output.

The implementation preserves the hardware FIFO order: after three iterations, the three processed source colors occupy RGB0, RGB1 and RGB2 in their original order. FLAG conditions are accumulated across all three operations.

## Regression coverage

The host test initializes three distinct RGB FIFO entries and uses IR0=0 to verify that DPCT consumes and returns all three colors in FIFO order, with the RGBC CODE byte applied.

## Validation scope

This milestone is source-level GTE implementation with host-side regression coverage. It does not constitute a Dreamcast/KallistiOS build, BIOS boot, CDI generation, Flycast gameplay test or GDEMU validation.
