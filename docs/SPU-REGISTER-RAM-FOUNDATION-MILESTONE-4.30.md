# SPU register and sample-RAM foundation — milestone 4.30

This milestone introduces the first isolated PlayStation SPU device model and connects its register window to the CPU-facing machine bus.

## Implemented

- A dedicated `PsxSpu` device with 512 KiB of sample RAM.
- A 0x200-byte register window at `0x1f801c00–0x1f801dff`.
- Little-endian 16-bit register reads and writes, plus byte-lane access.
- SPU transfer-address register at offset `0x1a6`, expressed in 8-byte units.
- SPU transfer-data port at offset `0x1a8`; each 16-bit access advances by two bytes.
- Transfer address wraps at the 512 KiB RAM boundary.
- Machine initialization/reset and CPU bus routing for 8-, 16- and 32-bit accesses.
- Host tests for register storage, transfer-port sequencing, byte lanes, RAM wrapping, reset and bus integration.

## Deliberate scope limits

This is a register/RAM foundation, not a working sound synthesizer. Voice registers are currently stored as ordinary register words; key-on/key-off do not yet start voices. There is no ADSR envelope, pitch stepping, ADPCM voice decoder, interpolation, noise generator, reverb, CD audio mixing, SPU IRQ generation, DMA channel 4, or Dreamcast audio output.

The status register is a placeholder read value. Transfer timing and hardware busy/ready behavior are not modeled. Register side effects beyond the sample-RAM transfer port are not yet implemented.

## Validation

The SPU module has its own `test_spu` target, and `test_bus` exercises it through the shared CPU bus. CI validation is pending for this commit; no test result is claimed until GitHub Actions completes.

## Next steps

1. Confirm this register/RAM layer with CI.
2. Implement SPU control/status semantics and key-on/key-off latches.
3. Add voice state and deterministic sample stepping.
4. Implement ADPCM block decoding and ADSR before connecting audio output.
