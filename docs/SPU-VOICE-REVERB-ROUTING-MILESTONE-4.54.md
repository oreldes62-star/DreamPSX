# SPU per-voice reverb routing and wet/dry mixing milestone 4.54

## Scope

The SPU sample mixer reads the hardware reverb-enable bitmap at 0x1F801D98/0x1F801D9A and routes only selected voices to the stereo reverb-send bus. Voices 0-15 use the low halfword; voices 16-23 use bits 0-7 of the high halfword.

Each voice's send signal is taken after its envelope and fixed left/right volume scaling, so the dry and wet paths share the same per-voice level. The dry sum and selected send sum remain wide while voices are accumulated. The send is clamped only at the 16-bit DSP input boundary; the wet return is then added to the dry output and saturated once at the final stereo output.

The existing SPUCNT master reverb-enable gate remains authoritative inside the wet DSP. When it is disabled, the mixer remains dry-only and the reverb cursor does not advance. When enabled, the DSP is clocked once for every mixed sample, including samples with no voices selected for reverb.

## Regression coverage added

- Voice 0 is routed by the low enable bitmap.
- Voice 23 is routed by bit 7 of the high enable bitmap.
- With a voice's send bit clear, the dry signal remains audible while the reverb reflection input remains silent.
- With the send bit set, the voice contributes to the reverb work area.
- The reverb cursor advances exactly once per mixed sample while the master reverb gate is enabled.

## Validation

- GitHub Actions host CMake build: passed.
- CTest: 44/44 tests passed, including SPU routing regressions.
- KallistiOS toolchain build: passed. All 23 core/platform translation units were compiled into the Dreamcast static archive, and the audio smoke-test executable was verified as ELF32, little-endian, Renesas SuperH, with entry point 0x8c010000.
- The `dreampsx-dreamcast-build` artifact was uploaded by GitHub Actions (run 46073981985; artifact ID 15234367251).

## Remaining limitations

This milestone connects the existing host-side reverb DSP to voice mixing; it does not establish cycle-accurate SPU behavior. Sweep-mode voice volumes, exact hardware reverb address arithmetic/timing, SPU IRQ behavior, and final dry/wet gain semantics still require reference-based validation. The ELF has not yet been executed in Flycast or on physical Dreamcast hardware. No runnable Dreamcast CDI image is currently validated.
