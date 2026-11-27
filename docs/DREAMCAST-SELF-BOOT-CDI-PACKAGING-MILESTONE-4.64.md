# Self-booting CDI packaging — milestone 4.64

## Objective

Turn the verified KallistiOS ELF build into a self-booting Dreamcast CDI artifact that can be opened by Flycast on Android. This milestone validates the Dreamcast disc-image packaging pipeline; it does not claim that a PS1 title is playable.

## Packaging pipeline

The new `platform/dreamcast/audio-smoke/package-cdi.sh` script uses the open-source `mkdcdisc` utility. It:

1. Checks that both DreamPSX ELF outputs exist.
2. Stages the separate PS1 file-backed boot diagnostic ELF and a README on the data track.
3. Invokes `mkdcdisc -e` with the AICA audio diagnostic ELF as the boot executable.
4. Uses `mkdcdisc` to generate the Dreamcast self-boot bootstrap, scrambled executable, ISO data track and CDI container.

GitHub Actions builds `mkdcdisc` from its upstream source using Meson and libisofs, then attempts to publish the artifact named `dreampsx-flycast-cdi`, containing `dreampsx-audio-smoke.cdi`.

## What this CDI currently does

The boot target is deliberately the existing AICA audio diagnostic. When launched, it should produce the 440 Hz stereo test tone. This is a useful first Flycast test of the self-boot image and Dreamcast executable startup path.

The CDI is not yet a playable PS1 emulator. The separate `DREAMPSX_BOOT.ELF` is the current PS1 file-backed startup diagnostic, but it is not selected as the disc boot executable in this first package.

## BIOS and game files

No PlayStation BIOS or game image is copied into the repository or generated artifact. The next boot-smoke CDI variant will need the user's own files at:

- `/cd/PSX/BIOS.BIN` — exactly 524,288 bytes
- `/cd/PSX/GAME.CUE`
- The relative BIN/ISO file referenced by the CUE (the current smoke loader expects a supported single-track data disc)

## Validation boundary

A successful CI packaging job proves that the image was generated and is non-empty. It does not prove that Flycast Android boots it or that a PS1 game runs. The first runtime check must be performed in Flycast; GPU presentation and controller input are still not wired into the current boot smoke application.
