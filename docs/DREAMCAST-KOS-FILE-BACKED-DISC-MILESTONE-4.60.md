# KallistiOS file-backed disc reader — milestone 4.60

## Objective

Connect the generic random-access disc source (milestone 4.59) to the Dreamcast filesystem so a raw PS1 disc image can be read sector-by-sector without allocating a full BIN/ISO buffer.

## Implementation

- Added `PsxKosDiscFile`, which owns a KallistiOS VFS file handle and its measured file size.
- Added `psx_kos_disc_file_open()`: opens a path read-only with `fs_open()`, obtains its size with `fs_total()`, and mounts it through `psx_disc_mount_reader()`.
- Added `psx_kos_disc_file_read_at()`: seeks to the requested byte offset and loops over `fs_read()` until the complete range is filled. Short reads are handled; EOF, seek errors and out-of-range requests fail cleanly.
- Added `psx_kos_disc_file_close()`, which unmounts the disc before closing the underlying file to prevent a dangling callback context.
- Added host-side VFS stubs and tests for partial reads, sector reads, seek failures, bounds, open failures and malformed file sizes.
- Added the adapter to the Dreamcast KallistiOS Makefile and CI executable archive verification.

## Platform constraints

The adapter deliberately serializes seek/read operations on one file handle. The same file must not be accessed concurrently by another thread while mounted. Offsets above `INT32_MAX` are rejected because this implementation uses KallistiOS `fs_seek()` and its `off_t` offset; ordinary PS1 CD images are below this limit.

## Current limitations

- The adapter currently opens a raw BIN/ISO path when the track mode is supplied by the caller. It does not yet parse a CUE file and resolve its relative FILE path automatically.
- It uses KallistiOS VFS file access, not low-level raw GD-ROM sector DMA. This is intentional for file-backed images stored on a mounted filesystem.
- Multi-track discs, CDDA track scheduling and subchannel data remain future work.
- No Flycast runtime, physical Dreamcast or GDEMU playback is claimed. This is a build-and-host-test integration milestone, not a bootable CDI release.

## Validation

- Host build and CTest passed on commit `03dcafb3c76e721d16e7e93960d3b8f3444f1a9f`: workflow run 46079023720, 45/45 tests passed.
- The pinned KallistiOS/Dreamcast build for the adapter was still running at documentation update time (workflow run 46079023677); its final result is not yet claimed.
