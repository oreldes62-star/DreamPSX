# Random-access disc sector source — milestone 4.59

## Objective

Remove the requirement that a PlayStation disc image be resident as one contiguous RAM allocation before the emulated CD-ROM can read it. This is a foundational step for the Dreamcast's 16 MB memory budget.

## Implementation

- Added the `PsxDiscReadAt` callback contract: a source receives a byte offset and requested length, fills the complete range, and reports success or failure.
- Added `psx_disc_mount_reader()` for raw single-track MODE1/2048, MODE1/2352 and MODE2/2352 sources. It records source size and sector geometry without allocating or copying image contents.
- Refactored sector reads to fetch only the requested 2048- or 2352-byte raw sector into a bounded local buffer, then validate the sector header and XA subheader as before.
- Preserved the existing caller-owned in-memory mounting path.
- Updated the emulated CD-ROM mount validation to accept either an in-memory source or a random-access reader.
- Added a sparse 64 MiB source regression test proving that one requested sector causes exactly one 2048-byte read, not a whole-image copy.
- Added CD-ROM integration coverage mounting a reader-backed disc and issuing ReadN through the emulated CD-ROM registers.

## Memory behavior

The new raw-sector reader uses a 2352-byte temporary buffer per sector read. It does not allocate storage proportional to image size. The callback context and backing storage are owned by the platform/frontend.

## Current limitations

- The new reader mount API accepts explicit raw track mode and source size. Parsing a CUE file and resolving its BIN filename into a platform file handle is a follow-up integration task.
- No Dreamcast filesystem adapter is wired yet; no KOS/GDEMU image is claimed.
- Multi-track CUE layouts, CDDA track scheduling and subchannel data remain future work.
- This milestone does not yet boot a PS1 BIOS or game.

## Validation status

The changes and regression tests have been committed. A fresh CI run for milestone 4.59 has not yet been observed; no new test pass or Dreamcast build is claimed here.
