# GPU IRQ1 request and CPU routing — milestone 4.07

## Delivered

- Implemented GP0(1Fh), which sets the GPU interrupt request flag (GPUSTAT bit 24).
- Implemented GP1(02h), which acknowledges and clears the GPU's IRQ request.
- Exposed the GPU IRQ output as a read-only device signal.
- Connected that signal through the system bus to interrupt-controller source 1.
- Preserved the distinction between the device source level and the latched I_STAT request: acknowledging the GPU lowers the source line, while software still acknowledges I_STAT separately.
- Added tests for GPUSTAT request/acknowledge, bus routing and pending-bit latching, and delivery through COP0 IP2 to a CPU interrupt exception at an instruction boundary.

## Scope

This milestone implements the GPU command and interrupt path only. It does not add GPU command timing, interrupt latency in cycles, or cycle-accurate raster scheduling.

## Validation

- GitHub Actions run `46046065924`: host configure and build succeeded; CTest passed all 33/33 tests.
