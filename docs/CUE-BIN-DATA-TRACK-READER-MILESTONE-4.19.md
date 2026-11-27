# CUE/BIN data-track reader foundation — milestone 4.19

## Delivered

- Added `PsxDiscImage` as a non-owning, bounded view over a caller-provided BIN buffer.
- Added a strict single-file/single-track CUE parser. It requires FILE BINARY, TRACK 01 and INDEX 01.
- Supported track layouts:
  - MODE1/2048: direct 2048-byte user-data sectors.
  - MODE1/2352: extracts the 2048-byte payload at offset 16 after checking the raw-sector sync pattern and mode byte.
  - MODE2/2352: extracts the 2048-byte Form 1 payload at offset 24 after checking sync, mode and duplicated subheader. Form 2 sectors are rejected.
- INDEX 01 is converted to a source-sector offset, so LBA 0 addresses the first sector of the track rather than the beginning of any pregap.
- Reads are bounded by the track's available sector count. The caller retains ownership of the BIN buffer.

## Validation

GitHub Actions host build and CTest passed: 39/39 tests. The new `test_disc_image` covers CRLF parsing, INDEX 01 offsets, MODE1/2048, MODE1/2352, MODE2/2352 Form 1 extraction, Form 2 rejection, out-of-range reads, malformed CUE sheets and unmounting.

## Deliberate limitations

- This is a parser and sector extraction foundation, not yet a full disc-set manager.
- Multi-file CUE sheets, multiple tracks, audio tracks, PREGAP/INDEX 00 semantics, WAV files, CDI containers and filesystem path loading are not implemented.
- The parser accepts the CUE text and BIN bytes supplied by the frontend; it does not open files itself.
- The new reader is not yet connected to the CD-ROM controller. Existing CD-ROM mounting still uses contiguous 2048-byte ISO data. That adapter is the next integration task.
- No Dreamcast build, CDI packaging, Flycast, GDEMU or physical hardware validation is claimed.
