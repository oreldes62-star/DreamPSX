# Single-track CUE loader — milestone 4.61

## Objective

Complete the first file-backed disc mount path by loading a CUE sheet from the Dreamcast VFS, resolving its referenced BIN/ISO path, and applying INDEX 01 as the data-track start sector.

## Implementation

- Added \`psx_kos_disc_file_open_cue()\` to the KallistiOS file adapter.
- Reads the CUE file through VFS in a bounded 4096-byte buffer and loops over short reads.
- Parses one quoted \`FILE ... BINARY\`, one \`TRACK 01\`, and one \`INDEX 01 mm:ss:ff\`.
- Supports the existing raw track modes MODE1/2048, MODE1/2352 and MODE2/2352.
- Resolves relative image filenames against the directory containing the CUE file; absolute slash-prefixed paths are preserved.
- Opens the resolved BIN/ISO through the existing random-access reader and adjusts the mounted descriptor to start at INDEX 01.
- Host VFS tests exercise short CUE reads, relative path resolution, INDEX 01 sector skipping, and actual sector retrieval from the mounted image.

## Deliberate first-version limits

- Single FILE / single TRACK 01 only. Multi-track CUE sheets are rejected.
- Only BINARY data tracks in the three existing raw modes are supported. AUDIO, WAVE, PREGAP synthesis, subchannel data and CDDA playback are not implemented.
- CUE sheets larger than 4096 bytes and lines longer than 255 characters are rejected.
- This is a CUE-to-file mount layer, not yet a full PS1 disc boot test or CDI image.

## Validation

- Host build and CTest passed on commit `ef1149334a49e06f936483d583bdff9f83eb8ff3`: workflow run 46079477092, 45/45 tests passed, including the CUE-to-BIN sector-read test.
- The pinned KallistiOS/Dreamcast build passed on the same commit: workflow run 46079477119. It compiled the CUE-enabled platform adapter, verified the SH-4 ELF and uploaded the Dreamcast build artifact.
