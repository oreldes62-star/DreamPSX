# DMA transfer event telemetry — milestone 4.06

## Delivered

- Added a compact last-transfer event snapshot to the DMA controller.
- Each endpoint attempt records a monotonically increasing sequence number, channel, CHCR sync mode, transfer direction, successfully transferred payload-word delta, linked-list-header delta, and completion result.
- The bus captures cumulative counters immediately before dispatch and records the deltas after the endpoint returns.
- Failed endpoint attempts are recorded too; their event reports only work completed before the failure and sets `completed=false`.
- The snapshot API returns false before the first event and for invalid arguments.
- Integration coverage verifies the initial empty state, completed channel-2 manual transfer metadata, and a failed out-of-range RAM-source attempt. The failed event reports zero payload words and leaves the CHCR request queued for retry.

## Timing boundary

Events describe completed work and dispatch outcomes. They do not assign cycles, infer bus occupancy, or claim cycle-accurate CPU/DMA overlap. This event boundary is intended to provide reliable inputs for a later scheduler.

## Validation

- GitHub Actions run `46045743670`: host configure and build succeeded; CTest passed all 33/33 tests.
- The failure-path fixture uses `0x00800000`, just beyond the PS1 RAM mirror window; the earlier `0x00200000` address was valid through RAM mirroring and therefore did not exercise an invalid read.
