# Self-booting CDI packaging — milestone 4.64

## Objective

Turn the verified KallistiOS ELF build into a self-booting Dreamcast CDI artifact that can be opened by Flycast on Android. This milestone validates the Dreamcast disc-image packaging pipeline; it does not claim that a PS1 title is playable.

## Automated audio diagnostic CDI

The `platform/dreamcast/audio-smoke/package-cdi.sh` script uses the open-source `mkdcdisc` utility. It checks both DreamPSX ELF outputs, stages the separate PS1 file-backed boot diagnostic ELF, and invokes `mkdcdisc -e` with the AICA audio diagnostic ELF as the boot executable. The resulting `dreampsx-audio-smoke.cdi` is published by GitHub Actions as artifact `dreampsx-flycast-cdi`.

The disc boots the AICA audio diagnostic, which should produce a 440 Hz stereo test tone. A successful CI build confirms packaging and artifact creation, not runtime execution in Flycast.

## Optional user-file-backed PS1 boot CDI

The same script can create a second, personalized image when run locally with `DREAMPSX_PSX_DIR` set to a directory containing:

- `BIOS.BIN` — exactly 524,288 bytes
- `GAME.CUE`
- The relative BIN/ISO file referenced by the CUE (the current loader supports a bounded single-track data CUE)

Example on a development computer:

```sh
DREAMPSX_PSX_DIR=/path/to/your/PSX sh platform/dreamcast/audio-smoke/package-cdi.sh
```

This produces `dreampsx-ps1-boot-smoke.cdi`, booting `dreampsx-boot-smoke.elf` with the user files staged under `/cd/PSX/`. The BIOS and game are neither committed nor uploaded by CI. The automated CI run deliberately skips this personalized image because it has no user BIOS/game files.

## Current limitations

The PS1 boot smoke app loads the BIOS, mounts the CUE/BIN through KOS VFS, resolves SYSTEM.CNF, loads the PS-X EXE, initializes PC/GP/SP, advances CPU instructions and starts AICA audio. It does not execute the full retail BIOS startup sequence. GPU frame presentation and controller input are not wired into the app, so this is not yet a playable PS1 emulator frontend.

## Validation boundary

The first automated audio CDI was generated successfully in GitHub Actions run 46081255394; the workflow completed with success and uploaded artifact `dreampsx-flycast-cdi` (3,566,765 bytes). Flycast Android runtime playback has not yet been verified. The optional personalized PS1 boot CDI packaging path is newly added and still needs its own CI/shell validation and a user-supplied test set.
