# Self-booting CDI packaging — milestone 4.64

## Objective

Turn the verified KallistiOS ELF build into a self-booting Dreamcast CDI artifact that can be opened by Flycast on Android. This milestone validates the Dreamcast disc-image packaging chain; it does not claim that a PS1 title is playable.

## Packaging pipeline

The new `platform/dreamcast/audio-smoke/package-cdi.sh` script:

1. Requires the KallistiOS/Dreamcast tools `sh-elf-objcopy`, `scramble`, `makeip`, `mkisofs` and `cdi4dc`.
2. Converts `dreampsx-audio-smoke.elf` to a raw SH-4 binary, excluding the KOS stack section.
3. Scrambles that binary into the Dreamcast boot filename `1ST_READ.BIN`.
4. Generates an `IP.BIN` bootstrap with the KallistiOS `makeip` utility.
5. Builds the ISO data session with the Dreamcast boot LBA parameters and converts it to CDI with `cdi4dc`.
6. Keeps `dreampsx-boot-smoke.elf` as a separate file on the disc for the next PS1 file-backed boot integration stage.

GitHub Actions now attempts to produce and upload the artifact named `dreampsx-flycast-cdi`, containing `dreampsx-audio-smoke.cdi`.

## What this CDI currently does

The boot target is deliberately the existing AICA audio diagnostic. When launched, it should produce the 440 Hz stereo test tone. This is a useful first Flycast test of the self-boot image, SH-4 executable conversion, IP.BIN, ISO session and CDI packaging.

The CDI is not yet a playable PS1 emulator. The separate `DREAMPSX_BOOT.ELF` is the current PS1 file-backed startup diagnostic, but it is not selected as the disc boot executable in this first package.

## BIOS and game files

No PlayStation BIOS or game image is copied into the repository or generated artifact. The next boot-smoke CDI variant will need the user's own files at:

- `/cd/PSX/BIOS.BIN` — exactly 524,288 bytes
- `/cd/PSX/GAME.CUE`
- The relative BIN/ISO file referenced by the CUE (the current smoke loader expects a supported single-track data disc)

## Validation boundary

A successful CI packaging job proves that the image was generated and is non-empty. It does not prove that Flycast Android boots it or that a PS1 game runs. The first runtime check must be performed in Flycast; GPU presentation and controller input are still not wired into the current boot smoke application.
