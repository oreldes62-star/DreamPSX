# SPU reverb comb-filter milestone 4.51

## Scope

This milestone adds the four-tap stereo comb stage to the host-side SPU reverb model. The left and right channels read their own four programmable delay offsets (MLCOMB1-4 and MRCOMB1-4); each tap is multiplied by its corresponding signed Q15 VCOMB coefficient. Products are accumulated at full width and the final channel outputs are saturated to signed 16-bit samples.

The stage is deliberately a read/compute operation: it does not advance the shared work-area cursor or overwrite delay-line samples. This keeps filter evaluation deterministic and allows the surrounding reverb scheduler to control memory writes and cursor advancement explicitly.

## Regression coverage

The SPU test suite verifies:

1. Four independent left and right delay taps are read from their configured work-RAM offsets.
2. Signed Q15 coefficients produce the expected positive and negative stereo sums.
3. Accumulation saturates only at the final signed 16-bit output.
4. Processing does not advance the reverb work-area cursor.
5. Null arguments are rejected.

## Limitations

This is not yet a complete PS1 reverb implementation. IIR/comb/APF scheduling, the two all-pass stages, output-volume routing, reverb enable gating, exact work-area wrap semantics and cycle-accurate timing still need integration and validation. Host tests do not establish compatibility on Flycast or physical Dreamcast hardware.
