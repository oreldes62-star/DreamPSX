# SPU reverb all-pass milestone 4.52

## Scope

Adds the two cascaded stereo all-pass stages (APF1 and APF2) after the comb filter. Each channel reads the delayed sample at the programmed mAPF-dAPF relative address, writes the feedback term at mAPF with signed 16-bit saturation, and computes the forward term using the signed Q15 APF coefficient. APF2 consumes APF1's output.

The work-area cursor is not advanced here; scheduling remains the caller's responsibility.

## Regression coverage

Tests cover both channels through both stages, the feedback values written to all four mAPF destinations, signed samples, output values, cursor stability, and invalid arguments.

## Reference and limitations

Equations and register ordering follow psx-spx, section 7.11: https://psx-spx.consoledev.net/soundprocessingunitspu/

This is not yet a complete hardware-accurate reverb engine. Scheduling, disabled-reverb behavior, output-volume routing, exact wrap and cycle timing, and validation on Flycast/real Dreamcast remain outstanding.
