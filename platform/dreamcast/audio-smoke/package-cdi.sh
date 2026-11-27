#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$SCRIPT_DIR/cdi-build"
ISO_ROOT="$BUILD_DIR/iso-root"
OUT_DIR="${DREAMPSX_CDI_OUT_DIR:-$SCRIPT_DIR/cdi-output}"
AUDIO_ELF="${DREAMPSX_AUDIO_ELF:-$SCRIPT_DIR/dreampsx-audio-smoke.elf}"
BOOT_ELF="${DREAMPSX_BOOT_ELF:-$SCRIPT_DIR/dreampsx-boot-smoke.elf}"
TITLE="${DREAMPSX_CDI_TITLE:-DreamPSX Audio Test}"

command -v mkdcdisc >/dev/null 2>&1 || {
    echo "ERROR: mkdcdisc is required; install it with Meson and libisofs" >&2
    exit 127
}

[ -s "$AUDIO_ELF" ] || { echo "ERROR: missing audio smoke ELF: $AUDIO_ELF" >&2; exit 1; }
[ -s "$BOOT_ELF" ] || { echo "ERROR: missing boot smoke ELF: $BOOT_ELF" >&2; exit 1; }

rm -rf "$BUILD_DIR"
mkdir -p "$ISO_ROOT" "$OUT_DIR"

# This first disc is a Dreamcast boot-chain/audio diagnostic, not a playable
# PS1 emulator. No copyrighted BIOS or game data is bundled.
cp "$BOOT_ELF" "$ISO_ROOT/DREAMPSX_BOOT.ELF"

cat > "$ISO_ROOT/README.TXT" <<'EOF'
DreamPSX Dreamcast CDI diagnostic build

This disc boots the AICA 440 Hz stereo audio smoke test.
DREAMPSX_BOOT.ELF is the separate PS1 file-backed boot diagnostic ELF.
This is not yet a playable PS1 emulator frontend.

The PS1 boot diagnostic expects user-provided files at:
  /cd/PSX/BIOS.BIN  (exactly 524288 bytes)
  /cd/PSX/GAME.CUE
  The relative BIN/ISO file named by GAME.CUE

BIOS and game files are not included.
EOF

mkdcdisc -e "$AUDIO_ELF" -D "$ISO_ROOT" -n "$TITLE" \
    -o "$OUT_DIR/dreampsx-audio-smoke.cdi"

[ -s "$OUT_DIR/dreampsx-audio-smoke.cdi" ] || {
    echo "ERROR: CDI output was not created" >&2
    exit 1
}

echo "Created: $OUT_DIR/dreampsx-audio-smoke.cdi"
