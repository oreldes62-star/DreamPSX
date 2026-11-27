# DreamPSX

PlayStation emulator research and development for Sega Dreamcast.

## Development status
Experimental PlayStation emulator research project targeting Sega Dreamcast. Current work includes a bounded 4096-frame interleaved stereo PCM queue with overflow/underrun telemetry, connected to the machine-level SPU rendering API and scheduled at 44.1 kHz from the shared PS1 clock; a host-tested KallistiOS-compatible PCM callback bridge and AICA stream lifecycle wrapper; and a diagnostic KallistiOS audio application. The real KallistiOS SH-4 toolchain has compiled all 23 core/platform translation units into a Dreamcast static archive and produced a verified ELF32 SuperH audio smoke-test executable (latest run 46075990827, artifact 15234783723). SPU milestones 4.51–4.56 now cover stereo reverb filtering/routing, direct and swept per-voice volumes, and independent master stereo volume; the master-volume sweep-entry correction passes all 44 host CTest cases (run 46075990832), and the matching KallistiOS cross-build succeeds. Other implemented areas include SYSTEM.CNF-driven ISO executable loading, integrated CD-ROM mounting and CUE/BIN data-sector reading, GPU primitives and display scanout, GPU IRQ1 routing, CPU/GTE timing foundations, timers, GPU and CD-ROM DMA modes, and DICR master interrupt routing to PS1 IRQ source 3. DMA counters track successfully transferred payload words by workload; linked-list packet headers remain a separate metadata metric, and these counters are not hardware cycle counts. The implementation remains synchronous in several hardware paths. CD-ROM ReadN sector delivery advances from emulated system cycles at the Setmode-selected normal/double-speed cadence; Setmode, delayed Pause/Stop completion, and IRQ2 routing are modeled, while drive seek/command timing and hardware overrun buffering remain simplified. The AICA smoke executable is a diagnostic tone application, not yet a PS1-game audio runtime. The stream callback now follows the pinned KallistiOS byte-count contract (total interleaved bytes, not frames); milestone 4.58 names and tests the 2048-byte KOS stream allocation contract, while the machine-SPU-to-callback integration test verifies stereo delivery; all 44 host CTest cases pass (run 46077164459). The corrected callback and core also passed the KallistiOS SH-4 build/ELF verification (run 46076903036, artifact 15234764907); the latest KallistiOS SH-4 build also passed, compiling all 23 core/platform translation units, linking the AICA smoke ELF, verifying ELF32/SuperH and uploading artifact 15235761926 (run 46077164540). It has not been auditioned in Flycast or on physical hardware. No runnable Dreamcast CDI image is currently validated. Host tests and cross-compilation alone do not establish Dreamcast performance or game compatibility.

## Current milestones
- [Dreamcast AICA stream buffer byte contract milestone 4.58](docs/DREAMCAST-AICA-BUFFER-BYTE-CONTRACT-MILESTONE-4.58.md)
- [Dreamcast AICA byte-count callback integration milestone 4.57](docs/DREAMCAST-AICA-BYTE-CALLBACK-INTEGRATION-MILESTONE-4.57.md)
- [SPU per-voice volume control and sweep milestone 4.55](docs/SPU-VOICE-VOLUME-SWEEP-MILESTONE-4.55.md)
- [SPU per-voice reverb routing and wet/dry mixing milestone 4.54](docs/SPU-VOICE-REVERB-ROUTING-MILESTONE-4.54.md)
- [SPU deterministic silence block validation milestone 4.49](docs/SPU-DETERMINISTIC-SILENCE-BLOCK-MILESTONE-4.49.md)
- [PCM queue edge-case validation milestone 4.48](docs/PCM-QUEUE-EDGE-CASE-TESTS-MILESTONE-4.48.md)
- [AICA smoke-test telemetry milestone 4.46](docs/DREAMCAST-AICA-SMOKE-TELEMETRY-MILESTONE-4.46.md)
- [Full DreamPSX core SH-4 cross-build milestone 4.47](docs/DREAMCAST-FULL-CORE-SH4-CROSS-BUILD-MILESTONE-4.47.md)
- [Dreamcast AICA audio prefill and pacing milestone 4.45](docs/DREAMCAST-AICA-PREFILL-PACING-MILESTONE-4.45.md)
- [Dreamcast AICA audio smoke-test milestone 4.44](docs/DREAMCAST-AICA-AUDIO-SMOKE-TEST-MILESTONE-4.44.md)
- [KallistiOS AICA stream lifecycle milestone 4.43](docs/KALLISTIOS-AICA-STREAM-LIFECYCLE-MILESTONE-4.43.md)
- [Dreamcast audio stream callback bridge milestone 4.42](docs/DREAMCAST-AUDIO-STREAM-BRIDGE-MILESTONE-4.42.md)
- [Cycle-locked SPU audio cadence milestone 4.41](docs/SPU-CYCLE-LOCKED-AUDIO-CADENCE-MILESTONE-4.41.md)
- [Machine SPU audio queue integration milestone 4.40](docs/MACHINE-SPU-AUDIO-QUEUE-MILESTONE-4.40.md)
- [Stereo audio ring buffer milestone 4.39](docs/STEREO-AUDIO-RING-BUFFER-MILESTONE-4.39.md)
- [SPU interleaved PCM block output milestone 4.38](docs/SPU-PCM-BLOCK-OUTPUT-MILESTONE-4.38.md)
- [SPU 24-voice stereo mixer foundation milestone 4.37](docs/SPU-STEREO-MIXER-MILESTONE-4.37.md)
- [SPU ADSR envelope milestone 4.36](docs/SPU-ADSR-ENVELOPE-MILESTONE-4.36.md)
- [SPU pitch stepping milestone 4.35](docs/SPU-PITCH-STEPPING-MILESTONE-4.35.md)
- [SPU voice playback integration milestone 4.34](docs/SPU-VOICE-PLAYBACK-INTEGRATION-MILESTONE-4.34.md)
- [SPU ADPCM block decoder milestone 4.33](docs/SPU-ADPCM-BLOCK-DECODER-MILESTONE-4.33.md)
- [SPU per-voice register bank milestone 4.32](docs/SPU-PER-VOICE-REGISTERS-MILESTONE-4.32.md)
- [SPU key-on/key-off and control milestone 4.31](docs/SPU-KEY-ON-OFF-CONTROL-MILESTONE-4.31.md)
- [SPU register and sample-RAM foundation milestone 4.30](docs/SPU-REGISTER-RAM-FOUNDATION-MILESTONE-4.30.md)
- [XA timed PCM scheduling milestone 4.29](docs/XA-TIMED-PCM-SCHEDULING-MILESTONE-4.29.md)
- [CD-ROM XA PCM routing milestone 4.28](docs/CDROM-XA-PCM-ROUTING-MILESTONE-4.28.md)
- [Complete XA sector decoder milestone 4.27](docs/XA-COMPLETE-SECTOR-DECODER-MILESTONE-4.27.md)
- [CD-ROM XA Setfilter milestone 4.26](docs/CDROM-XA-SETFILTER-MILESTONE-4.26.md)
- [XA sound-group decoding milestone 4.25](docs/XA-SOUND-GROUP-DECODING-MILESTONE-4.25.md)
- [XA ADPCM decoder foundation milestone 4.24](docs/XA-ADPCM-DECODER-FOUNDATION-MILESTONE-4.24.md)
- [Raw CUE/BIN machine boot milestone 4.21](docs/RAW-CUE-BIN-MACHINE-BOOT-MILESTONE-4.21.md)
- [CUE/BIN CD-ROM integration milestone 4.20](docs/CUE-BIN-CDROM-INTEGRATION-MILESTONE-4.20.md)
- [CUE/BIN data-track reader foundation milestone 4.19](docs/CUE-BIN-DATA-TRACK-READER-MILESTONE-4.19.md)
- [Integrated boot-from-disc and CD-ROM mounting milestone 4.18](docs/INTEGRATED-BOOT-FROM-DISC-MILESTONE-4.18.md)
- [SYSTEM.CNF boot parser milestone 4.16](docs/SYSTEM-CNF-BOOT-PARSER-MILESTONE-4.16.md)
- [PS-X EXE loader foundation milestone 4.15](docs/PSX-EXE-LOADER-MILESTONE-4.15.md)
- [ISO-9660 reader foundation milestone 4.14](docs/ISO9660-READER-FOUNDATION-MILESTONE-4.14.md)
- [CD-ROM Setmode, Pause and Stop milestone 4.13](docs/CDROM-SETMODE-PAUSE-STOP-MILESTONE-4.13.md)
- [CD-ROM cycle-driven sector scheduler milestone 4.12](docs/CDROM-CYCLE-SCHEDULER-MILESTONE-4.12.md)
- [CD-ROM DMA channel 3 milestone 4.11](docs/CDROM-DMA-CHANNEL3-MILESTONE-4.11.md)
- [CD-ROM continuous ReadN streaming milestone 4.10](docs/CDROM-READN-STREAM-MILESTONE-4.10.md)
- [CD-ROM ISO ReadN sector delivery milestone 4.09](docs/CDROM-ISO-READN-SECTOR-MILESTONE-4.09.md)
- [GPU IRQ1 request and CPU routing milestone 4.07](docs/GPU-IRQ1-CPU-ROUTING-MILESTONE-4.07.md)
- [DMA transfer event telemetry milestone 4.06](docs/DMA-TRANSFER-EVENT-TELEMETRY-MILESTONE-4.06.md)
- [DMA per-mode workload counters milestone 4.05](docs/DMA-PER-MODE-WORKLOAD-COUNTERS-MILESTONE-4.05.md)
- [DMA transfer instrumentation milestone 4.04](docs/DMA-TRANSFER-INSTRUMENTATION-MILESTONE-4.04.md)
- [DMA bus ownership serialization milestone 4.03](docs/DMA-BUS-OWNERSHIP-MILESTONE-4.03.md)
- [DMA transfer lifecycle milestone 4.02](docs/DMA-TRANSFER-LIFECYCLE-MILESTONE-4.02.md)
- [DMA arbitration queue integration milestone 4.01](docs/DMA-ARBITRATION-QUEUE-INTEGRATION-MILESTONE-4.01.md)
- [DMA channel completion bookkeeping milestone 3.98](docs/DMA-COMPLETION-BOOKKEEPING-MILESTONE-3.98.md)
- [DMA request retention and arbitration separation milestone 3.99](docs/DMA-REQUEST-RETENTION-MILESTONE-3.99.md)
- [DMA request queue and priority take milestone 4.00](docs/DMA-REQUEST-QUEUE-MILESTONE-4.00.md)
- [DMA pending request discovery milestone 3.97](docs/DMA-PENDING-REQUEST-DISCOVERY-MILESTONE-3.97.md)
- [DPCR priority selector milestone 3.96](docs/DPCR-PRIORITY-SELECTOR-MILESTONE-3.96.md)
- [DPCR decoding foundation milestone 3.95](docs/DPCR-DECODING-FOUNDATION-MILESTONE-3.95.md)
- [DPCR-triggered GPU DMA milestone 3.94](docs/DPCR-TRIGGERED-GPU-DMA-MILESTONE-3.94.md)
- [Host validation recovery milestone 3.93](docs/HOST-VALIDATION-RECOVERY-MILESTONE-3.93.md)
- [Host continuous integration foundation milestone 3.92](docs/HOST-CI-MILESTONE-3.92.md)
- [DMA interrupt semantics hardening milestone 3.91](docs/DMA-IRQ-SEMANTICS-MILESTONE-3.91.md)
- [DMA master interrupt delivery milestone 3.90](docs/DMA-IRQ-DELIVERY-MILESTONE-3.90.md)
- [GPU DMA request/block mode milestone 3.89](docs/GPU-DMA-REQUEST-BLOCK-MILESTONE-3.89.md)
- [GPU DMA linked-list mode milestone 3.88](docs/GPU-DMA-LINKED-LIST-MILESTONE-3.88.md)
- [GPU VRAM readback through manual DMA milestone 3.87](docs/GPU-DMA-READBACK-MILESTONE-3.87.md)
- [First GPU DMA transfer: manual RAM-to-GP0 milestone 3.86](docs/GPU-DMA-MANUAL-RAM-TO-GP0-MILESTONE-3.86.md)
- [DMA register bank and bus integration milestone 3.85](docs/DMA-REGISTER-BANK-MILESTONE-3.85.md)
- [GP1 DMA direction and GPUSTAT request milestone 3.84](docs/GP1-DMA-DIRECTION-MILESTONE-3.84.md)
- [GPUSTAT readiness state milestone 3.83](docs/GPU-STATUS-READINESS-MILESTONE-3.83.md)
- [COP2 CU2 milestone 1.2](docs/COP2-CU2-MILESTONE-1.2.md)
- [COP2 BC2F/BC2T milestone 1.3](docs/COP2-BC2-MILESTONE-1.3.md)
- [COP2 LWC2/SWC2 milestone 1.4](docs/COP2-LWC2-SWC2-MILESTONE-1.4.md)
- [Memory milestone 1.5](docs/MEMORY-MILESTONE-1.5.md)
- [IRQ milestone 1.6](docs/IRQ-MILESTONE-1.6.md)
- [IRQ MMIO adapter milestone 1.8](docs/IRQ-MMIO-MILESTONE-1.8.md)
- [COP0 IRQ milestone 1.9](docs/COP0-IRQ-MILESTONE-1.9.md)
- [IRQ/COP0 integration milestone 2.0](docs/IRQ-COP0-INTEGRATION-MILESTONE-2.0.md)
- [Machine state milestone 2.1](docs/MACHINE-STATE-MILESTONE-2.1.md)
- [Memory backing store milestone 2.2](docs/MEMORY-BACKING-STORE-MILESTONE-2.2.md)
- [CPU bus core milestone 2.3](docs/BUS-CORE-MILESTONE-2.3.md)
- [Machine/bus integration milestone 2.4](docs/MACHINE-BUS-INTEGRATION-MILESTONE-2.4.md)
- [R3000A context and BIOS fetch milestone 2.5](docs/R3000A-CONTEXT-MILESTONE-2.5.md)
- [First R3000A execution milestone 2.6](docs/R3000A-EXECUTION-MILESTONE-2.6.md)
- [R3000A integer and memory expansion milestone 2.7](docs/R3000A-INTEGER-MEMORY-MILESTONE-2.7.md)
- [R3000A COP0 exception entry milestone 2.8](docs/R3000A-COP0-EXCEPTIONS-MILESTONE-2.8.md)
- [R3000A delay-slot tracking milestone 2.9](docs/R3000A-DELAY-SLOT-MILESTONE-2.9.md)
- [R3000A COP0 instruction support milestone 3.0](docs/R3000A-COP0-INSTRUCTIONS-MILESTONE-3.0.md)
- [Automatic hardware interrupt dispatch milestone 3.1](docs/AUTOMATIC-IRQ-DISPATCH-MILESTONE-3.1.md)
- [Synchronous CPU exception dispatch milestone 3.2](docs/SYNCHRONOUS-CPU-EXCEPTIONS-MILESTONE-3.2.md)
- [R3000A SYSCALL, BREAK and encoding validation milestone 3.3](docs/R3000A-SYSCALL-BREAK-VALIDATION-MILESTONE-3.3.md)
- [R3000A load delay slot milestone 3.4](docs/R3000A-LOAD-DELAY-MILESTONE-3.4.md)
- [R3000A LWL/LWR unaligned load milestone 3.5](docs/R3000A-LWL-LWR-MILESTONE-3.5.md)
- [R3000A memory access validation milestone 3.6](docs/R3000A-MEMORY-ACCESS-VALIDATION-MILESTONE-3.6.md)
- [R3000A delayed-load exception milestone 3.7](docs/R3000A-DELAYED-LOAD-EXCEPTION-MILESTONE-3.7.md)
- [Host build and automated test foundation milestone 3.8](docs/HOST-BUILD-MILESTONE-3.8.md)
- [R3000A REGIMM branch instructions milestone 3.9](docs/R3000A-REGIMM-BRANCHES-MILESTONE-3.9.md)
- [R3000A branch encoding validation milestone 3.10](docs/R3000A-BRANCH-ENCODING-MILESTONE-3.10.md)
- [R3000A REGIMM link-register aliasing milestone 3.11](docs/R3000A-REGIMM-LINK-ALIAS-MILESTONE-3.11.md)
- [R3000A REGIMM branch-delay exception milestone 3.12](docs/R3000A-REGIMM-DELAY-SLOT-MILESTONE-3.12.md)
- [R3000A delayed-load/control-flow milestone 3.13](docs/R3000A-DELAYED-LOAD-CONTROL-FLOW-MILESTONE-3.13.md)
- [R3000A delayed-load/link/exception milestone 3.14](docs/R3000A-DELAYED-LOAD-EXCEPTION-INTERACTIONS-MILESTONE-3.14.md)
- [R3000A SWL/SWR partial-word store milestone 3.15](docs/R3000A-SWL-SWR-MILESTONE-3.15.md)
- [R3000A variable shifts milestone 3.16](docs/R3000A-VARIABLE-SHIFTS-MILESTONE-3.16.md)
- [R3000A multiply/divide edge-case tests milestone 3.17](docs/R3000A-MULTIPLY-DIVIDE-VALIDATION-MILESTONE-3.17.md)
- [R3000A conditional trap instructions milestone 3.18](docs/R3000A-TRAP-INSTRUCTIONS-MILESTONE-3.18.md)
- [R3000A traps in branch delay slots milestone 3.19](docs/R3000A-TRAP-DELAY-SLOT-MILESTONE-3.19.md)
- [GTE register-file foundation milestone 3.20](docs/GTE-REGISTER-FILE-MILESTONE-3.20.md)
- [COP2 transfers and GTE memory access milestone 3.21](docs/COP2-TRANSFERS-MILESTONE-3.21.md)
- [GTE NCLIP command milestone 3.22](docs/GTE-NCLIP-MILESTONE-3.22.md)
- [GTE MVMVA command milestone 3.23](docs/GTE-MVMVA-MILESTONE-3.23.md)
- [GTE MVMVA selector validation milestone 3.24](docs/GTE-MVMVA-SELECTOR-VALIDATION-MILESTONE-3.24.md)
- [GTE RTPS perspective transformation milestone 3.25](docs/GTE-RTPS-MILESTONE-3.25.md)
- [GTE RTPT triple perspective transformation milestone 3.26](docs/GTE-RTPT-MILESTONE-3.26.md)
- [GTE AVSZ3/AVSZ4 depth averaging milestone 3.27](docs/GTE-AVSZ3-AVSZ4-MILESTONE-3.27.md)
- [GTE SQR register squaring milestone 3.28](docs/GTE-SQR-MILESTONE-3.28.md)
- [GTE OP outer product milestone 3.29](docs/GTE-OP-MILESTONE-3.29.md)
- [GTE GPF interpolation milestone 3.30](docs/GTE-GPF-MILESTONE-3.30.md)
- [GTE GPL accumulated interpolation milestone 3.31](docs/GTE-GPL-MILESTONE-3.31.md)
- [GTE NCS normal color milestone 3.32](docs/GTE-NCS-MILESTONE-3.32.md)
- [GTE NCCS normal color modulation milestone 3.33](docs/GTE-NCCS-MILESTONE-3.33.md)
- [GTE NCCT triple normal-color milestone 3.34](docs/GTE-NCCT-MILESTONE-3.34.md)
- [GTE NCT triple lighting milestone 3.35](docs/GTE-NCT-MILESTONE-3.35.md)
- [GTE DPCS and INTPL interpolation milestone 3.36](docs/GTE-DPCS-INTPL-MILESTONE-3.36.md)
- [GTE DPCT triple interpolation milestone 3.37](docs/GTE-DPCT-MILESTONE-3.37.md)
- [GTE DCPL lighting depth cue milestone 3.38](docs/GTE-DCPL-MILESTONE-3.38.md)
- [GTE NCDS and NCDT milestone 3.39](docs/GTE-NCDS-NCDT-MILESTONE-3.39.md)
- [GTE CDP milestone 3.40](docs/GTE-CDP-MILESTONE-3.40.md)
- [GTE CC milestone 3.41](docs/GTE-CC-MILESTONE-3.41.md)
- [GPU foundation milestone 3.42](docs/GPU-FOUNDATION-MILESTONE-3.42.md)
- [GPU MMIO and external VRAM milestone 3.43](docs/GPU-MMIO-MILESTONE-3.43.md)
- [GPU image transfer milestone 3.44](docs/GPU-IMAGE-TRANSFER-MILESTONE-3.44.md)
- [GPU flat-shaded triangle milestone 3.45](docs/GPU-FLAT-TRIANGLE-MILESTONE-3.45.md)
- [GPU drawing area and offset milestone 3.46](docs/GPU-DRAW-AREA-MILESTONE-3.46.md)
- [GPU shared-edge rasterization milestone 3.47](docs/GPU-SHARED-EDGE-MILESTONE-3.47.md)
- [GPU Gouraud triangle milestone 3.48](docs/GPU-GOURAUD-TRIANGLE-MILESTONE-3.48.md)
- [GPU raw 15-bit texture milestone 3.49](docs/GPU-RAW-TEXTURE-MILESTONE-3.49.md)
- [GPU indexed texture and CLUT milestone 3.50](docs/GPU-INDEXED-TEXTURE-CLUT-MILESTONE-3.50.md)
- [GPU modulated texture milestone 3.51](docs/GPU-MODULATED-TEXTURE-MILESTONE-3.51.md)
- [GPU semi-transparency milestone 3.52](docs/GPU-SEMITRANSPARENCY-MILESTONE-3.52.md)
- [GPU texture window milestone 3.53](docs/GPU-TEXTURE-WINDOW-MILESTONE-3.53.md)
- [GPU Gouraud-textured triangle milestone 3.54](docs/GPU-GOURAUD-TEXTURED-TRIANGLE-MILESTONE-3.54.md)
- [GPU Gouraud semi-transparent textured triangle milestone 3.55](docs/GPU-GOURAUD-SEMITRANSPARENT-TRIANGLE-MILESTONE-3.55.md)
- [GPU TPAGE blend-mode validation milestone 3.56](docs/GPU-TPAGE-BLEND-MODE-MILESTONE-3.56.md)
- [GPU textured triangle clipping milestone 3.57](docs/GPU-TEXTURED-CLIPPING-MILESTONE-3.57.md)
- [GPU texture and VRAM boundary validation milestone 3.58](docs/GPU-TEXTURE-VRAM-WRAP-MILESTONE-3.58.md)
- [GPU raw textured triangle correction milestone 3.59](docs/GPU-RAW-TEXTURE-MILESTONE-3.59.md)
- [GPU textured triangle opcode matrix milestone 3.60](docs/GPU-TEXTURED-OPCODE-MATRIX-MILESTONE-3.60.md)
- [GPU texture rasterization validation milestone 3.61](docs/GPU-TEXTURE-RASTERIZATION-MILESTONE-3.61.md)
- [GPU textured shared-edge validation milestone 3.62](docs/GPU-TEXTURE-SHARED-EDGES-MILESTONE-3.62.md)
- [GPU indexed texture boundary milestone 3.63](docs/GPU-INDEXED-TEXTURE-BOUNDARIES-MILESTONE-3.63.md)
- [GPU monochrome rectangle milestone 3.64](docs/GPU-MONOCHROME-RECTANGLES-MILESTONE-3.64.md)
- [GPU textured rectangle milestone 3.65](docs/GPU-TEXTURED-RECTANGLES-MILESTONE-3.65.md)

- [GTE command latency and result interlock milestone 3.81](docs/GTE-TIMING-INTERLOCK-MILESTONE-3.81.md)
- [Shared CPU clock synchronization milestone 3.82](docs/GTE-TIMING-INTERLOCK-MILESTONE-3.81.md)

## Target
Sega Dreamcast with 16 MiB RAM. Long-term target: CDI test build for Flycast, then GDEMU.

## Limitations
No Dreamcast/KallistiOS build, CDI image or playable game exists yet. CPU registers and BIOS instruction fetch are represented, and a growing subset of R3000A instructions plus the first GTE geometry commands (NCLIP, MVMVA, RTPS, RTPT, AVSZ3, AVSZ4, SQR, OP, GPF, GPL, NCS and NCCS initial implementations) execute; instruction decoding/execution remains partial, and the GPU now has a host-side VRAM/fill-rectangle core connected to GP0/GP1 MMIO, with external VRAM backing storage; the GPU now supports initial GP0 flat-shaded triangle rasterization and A0/C0 image transfers through GPUREAD; draw-mode, texture window, dithering, display output, DMA now supports synchronous channel-2 manual transfers in both directions, RAM-to-GP0 linked-list mode, and initial request/block transfers in both directions; cycle-accurate request stalls, complete CD-ROM command coverage (including XA/CDDA), and other hardware devices remain incomplete; textured triangle rendering supports fixed-color modulation, raw sampling and the four semi-transparency blend modes; textured triangles support fixed-color modulation and raw sampling; textured triangles now sample direct 15-bit, indexed 8-bit and indexed 4-bit textures through CLUT lookup; flat-shaded and Gouraud-shaded triangles are supported by the host GPU path; flat-triangle shared edges now use a deterministic top-left inclusion rule; GP0(E3h-E5h) drawing area and offset state now clips and translates flat triangles. A host CMake/CTest suite is defined and currently passes 41 tests. The SPU now has a host-side 24-voice stereo mixer and a fixed-size interleaved signed 16-bit stereo PCM block producer using ADPCM samples, pitch stepping, ADSR and fixed per-channel voice volumes; sweep mode, exact hardware sample scheduling, reverb, noise, pitch modulation, SPU DMA/IRQ, CD-audio mixing and a Dreamcast audio backend remain future work. No Dreamcast binary or CDI image is available yet.


GPU framebuffer output now includes a tested RGB555-to-RGB565 region conversion API; full GP1 display timing and mode emulation remain in progress. See [GPU framebuffer milestone 3.66](docs/GPU-FRAMEBUFFER-MILESTONE-3.66.md).


GP1 display-control register decoding (display origin, horizontal/vertical ranges, mode and enable state) is now exposed through the GPU display-state API. See [GPU display milestone 3.67](docs/GPU-DISPLAY-MILESTONE-3.67.md).


The display backend can now extract an RGB565 frame from the GP1-configured VRAM origin with wraparound and display-disable handling. See [GPU scanout milestone 3.68](docs/GPU-SCANOUT-MILESTONE-3.68.md).

Packed 24-bit display scanout is now available through the RGB565 framebuffer API. See [GPU scanout milestone 3.69](docs/GPU-SCANOUT-MILESTONE-3.69.md).

GP1 display mode decoding now exposes resolution, PAL/NTSC, color depth and interlace flags while synchronizing GPUSTAT. See [display mode milestone 3.70](docs/GPU-DISPLAY-MODE-MILESTONE-3.70.md).

The display API now also derives effective output width and height from GP1(06h/07h), including the hardware's four-pixel horizontal rounding and resolution-specific dot-clock divisors. See [display range milestone 3.71](docs/GPU-DISPLAY-RANGE-MILESTONE-3.71.md).

The GPU now exposes an explicit odd/even field state for 480-line interlace and mirrors it to GPUSTAT bit 31. See [interlace field milestone 3.72](docs/GPU-INTERLACE-FIELD-MILESTONE-3.72.md).

A first scanline scheduler now distinguishes NTSC and PAL field lengths, advances progressive frames and alternating interlaced fields, and reports VBlank transitions using GP1(07h). Half-line phase, CPU-clock integration and IRQ0 delivery remain future work. See [video timing milestone 3.73](docs/GPU-VIDEO-TIMING-MILESTONE-3.73.md).


The machine layer now routes GPU VBlank level transitions to the PS1 interrupt controller's IRQ0 source and refreshes COP0 Cause.IP2. The scanline clock is still explicitly advanced by the frontend; CPU-cycle scheduling and IRQ delivery during normal instruction stepping remain future work. See [VBlank IRQ milestone 3.74](docs/GPU-VBLANK-IRQ-MILESTONE-3.74.md).


The three root-counter register banks now expose counter, mode and target registers, read-to-clear target/overflow flags, and an explicit deterministic tick API. Clock-source selection, HBlank/dot-clock wiring and timer IRQ generation remain future work. See [timer register milestone 3.75](docs/TIMERS-REGISTER-MILESTONE-3.75.md).


Milestone 3.76 adds selectable root-counter clock domains and timer IRQ event routing. Milestone 3.77 emits one abstract HBlank pulse per advanced scanline and routes it to Timer1, including its IRQ path. Milestone 3.78 adds a shared system-cycle entry point and a persistent Timer2 divide-by-eight phase accumulator. Exact horizontal phase, dot-clock pulses, full synchronization modes and automatic CPU instruction-cycle feeding remain future integration work. See [HBlank/Timer1 integration milestone](docs/GPU-HBLANK-TIMER1-MILESTONE-3.77.md).


CPU stepping is now connected to the shared system-cycle domain: milestone 3.79 supplies a clearly marked one-cycle baseline per attempted instruction, allowing system-clock root counters and Timer2 /8 to advance during machine execution. Instruction-specific latency, bus wait states, dot-clock timing and cycle accuracy remain future work. See [CPU system-cycle integration milestone 3.79](docs/CPU-SYSTEM-CYCLE-INTEGRATION-MILESTONE-3.79.md).


Milestone 3.80 adds the R3000A multiply/divide unit's operand-dependent MULT/MULTU latency, fixed DIV/DIVU latency, deferred HI/LO results and instruction interlocks; independent instructions continue while the unit is busy and interlock waits feed the machine system clock. Full pipeline, cancellation, GTE and memory wait-state timing remain future work. See [R3000A HI/LO interlock milestone 3.80](docs/R3000A-HILO-INTERLOCK-MILESTONE-3.80.md).

SPU milestone 4.50 adds regression coverage for both ADPCM loop-start address capture and a software-programmed repeat address used without a loop-start marker. These host tests pass; cycle-accurate SPU timing and Flycast/physical Dreamcast validation remain outstanding. See [SPU ADPCM loop-address milestone 4.50](docs/SPU-ADPCM-LOOP-ADDRESS-MILESTONE-4.50.md).

SPU milestone 4.51 adds a four-tap stereo comb-filter stage with independent left/right delay addresses, signed Q15 coefficients, wide accumulation and final 16-bit saturation. Regression tests cover polarity, tap selection, saturation, cursor stability and invalid arguments. Full reverb scheduling, all-pass stages, output routing, hardware timing and Flycast/physical Dreamcast validation remain outstanding. See [SPU reverb comb-filter milestone 4.51](docs/SPU-REVERB-COMB-FILTER-MILESTONE-4.51.md).


Latest SPU milestone: 4.52 adds cascaded stereo APF1/APF2 all-pass processing (validation pending).


SPU milestone 4.55 adds signed direct voice volume and independently clocked left/right sweep control. Milestone 4.56 extends the same sweep model to the main stereo volume registers and applies master gain after dry/wet summation. Host regression coverage includes sweep cadence during silent output; see [SPU volume control milestones 4.55–4.56](docs/SPU-VOICE-VOLUME-SWEEP-MILESTONE-4.55.md). The earlier 4.55 host suite passed 44/44 and its Dreamcast KallistiOS/AICA smoke ELF was built and verified; the new 4.56 changes pass the host CMake build and CTest 44/44 (run 46075603036); the matching KallistiOS build and runtime testing remain pending.
