#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$SCRIPT_DIR/cdi-build"
ISO_ROOT="$BUILD_DIR/iso-root"
OUT_DIR="${DREAMPSX_CDI_OUT_DIR:-$SCRIPT_DIR/cdi-output}"
AUDIO_ELF="${DREAMPSX_AUDIO_ELF:-$SCRIPT_DIR/dreampsx-audio-smoke.elf}"
BOOT_ELF="${DREAMPSX_BOOT_ELF:-$SCRIPT_DIR/dreampsx-boot-smoke.elf}"
TITLE="${DREAMPSX_CDI_TITLE:-DreamPSX Audio Test}"

for tool in sh-elf-objcopy scramble makeip mkisofs cdi4dc; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "ERROR: required Dreamcast disc tool not found: $tool" >&2
        exit 127
    }
done

[ -s "$AUDIO_ELF" ] || { echo "ERROR: missing audio smoke ELF: $AUDIO_ELF" >&2; exit 1; }
[ -s "$BOOT_ELF" ] || { echo "ERROR: missing boot smoke ELF: $BOOT_ELF" >&2; exit 1; }

rm -rf "$BUILD_DIR"
mkdir -p "$ISO_ROOT" "$OUT_DIR"

# The first CDI is intentionally an audio/boot-chain diagnostic. It does not
# pretend to be a playable PS1 emulator and does not bundle copyrighted BIOS
# or game data.
sh-elf-objcopy -R .stack -O binary "$AUDIO_ELF" "$BUILD_DIR/audio-smoke.bin"
scramble "$BUILD_DIR/audio-smoke.bin" "$ISO_ROOT/1ST_READ.BIN"
cp "$BOOT_ELF" "$ISO_ROOT/DREAMPSX_BOOT.ELF"

cat > "$ISO_ROOT/README.TXT" <<'EOF'
DreamPSX Dreamcast CDI diagnostic build

This disc boots the AICA 440 Hz audio smoke test.
DREAMPSX_BOOT.ELF is the separate PS1 file-backed boot diagnostic ELF.
This is not yet a playable PS1 emulator frontend.

The PS1 boot diagnostic expects user-provided files at:
  /cd/PSX/BIOS.BIN  (exactly 524288 bytes)
  /cd/PSX/GAME.CUE
  /cd/PSX/GAME.BIN  (or the relative BIN filename named by GAME.CUE)

BIOS and game files are not included.
EOF

# KallistiOS makeip accepts command-line fields and writes a valid IP.BIN.
makeip -a JUE -b 1ST_READ.BIN -c "DreamPSX" -d 20261128 \
    -e V1.000 -g "$TITLE" -i CD-ROM1/1 -n T00000 -p E000F10 \
    "$BUILD_DIR/IP.BIN"

mkisofs -C 0,11702 -V DREAMPSX -G "$BUILD_DIR/IP.BIN" \
    -r -J -l -o "$BUILD_DIR/DREAMPSX.iso" "$ISO_ROOT"

cdi4dc "$BUILD_DIR/DREAMPSX.iso" "$OUT_DIR/dreampsx-audio-smoke.cdi"

[ -s "$OUT_DIR/dreampsx-audio-smoke.cdi" ] || {
    echo "ERROR: CDI output was not created" >&2
    exit 1
}

echo "Created: $OUT_DIR/dreampsx-audio-smoke.cdi"
