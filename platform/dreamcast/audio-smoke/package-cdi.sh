#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$SCRIPT_DIR/cdi-build"
AUDIO_ROOT="$BUILD_DIR/audio-iso-root"
PSX_ROOT="$BUILD_DIR/psx-iso-root"
OUT_DIR="${DREAMPSX_CDI_OUT_DIR:-$SCRIPT_DIR/cdi-output}"
AUDIO_ELF="${DREAMPSX_AUDIO_ELF:-$SCRIPT_DIR/dreampsx-audio-smoke.elf}"
BOOT_ELF="${DREAMPSX_BOOT_ELF:-$SCRIPT_DIR/dreampsx-boot-smoke.elf}"
PSX_INPUT_DIR="${DREAMPSX_PSX_DIR:-}"
TITLE="${DREAMPSX_CDI_TITLE:-DreamPSX}"

command -v mkdcdisc >/dev/null 2>&1 || {
    echo "ERROR: mkdcdisc is required; install it with Meson and libisofs" >&2
    exit 127
}

[ -s "$AUDIO_ELF" ] || { echo "ERROR: missing audio smoke ELF: $AUDIO_ELF" >&2; exit 1; }
[ -s "$BOOT_ELF" ] || { echo "ERROR: missing boot smoke ELF: $BOOT_ELF" >&2; exit 1; }

rm -rf "$BUILD_DIR"
mkdir -p "$AUDIO_ROOT" "$OUT_DIR"

# Disc 1: the known-good AICA audio diagnostic.
cp "$BOOT_ELF" "$AUDIO_ROOT/DREAMPSX_BOOT.ELF"
cat > "$AUDIO_ROOT/README.TXT" <<'EOF'
DreamPSX Dreamcast audio diagnostic.
This disc boots the AICA 440 Hz stereo test tone.
DREAMPSX_BOOT.ELF is a separate PS1 file-backed startup diagnostic.
It is not a playable PS1 emulator frontend.
EOF

mkdcdisc -e "$AUDIO_ELF" -D "$AUDIO_ROOT" -n "$TITLE Audio Test" \
    -o "$OUT_DIR/dreampsx-audio-smoke.cdi"
[ -s "$OUT_DIR/dreampsx-audio-smoke.cdi" ] || {
    echo "ERROR: audio CDI output was not created" >&2
    exit 1
}
echo "Created: $OUT_DIR/dreampsx-audio-smoke.cdi"

# Disc 2 is personalized and only built when the user supplies their own
# legally obtained BIOS and single-track CUE/BIN files in a local PSX folder.
if [ -n "$PSX_INPUT_DIR" ]; then
    [ -d "$PSX_INPUT_DIR" ] || {
        echo "ERROR: DREAMPSX_PSX_DIR is not a directory: $PSX_INPUT_DIR" >&2
        exit 1
    }
    [ -s "$PSX_INPUT_DIR/BIOS.BIN" ] || {
        echo "ERROR: missing user BIOS: $PSX_INPUT_DIR/BIOS.BIN" >&2
        exit 1
    }
    BIOS_SIZE=$(wc -c < "$PSX_INPUT_DIR/BIOS.BIN" | tr -d ' ')
    [ "$BIOS_SIZE" = "524288" ] || {
        echo "ERROR: BIOS.BIN must be exactly 524288 bytes (got $BIOS_SIZE)" >&2
        exit 1
    }
    [ -s "$PSX_INPUT_DIR/GAME.CUE" ] || {
        echo "ERROR: missing user disc descriptor: $PSX_INPUT_DIR/GAME.CUE" >&2
        exit 1
    }

    mkdir -p "$PSX_ROOT/PSX"
    cp -R "$PSX_INPUT_DIR/." "$PSX_ROOT/PSX/"
    cat > "$PSX_ROOT/README.TXT" <<'EOF'
DreamPSX PS1 file-backed boot diagnostic.
User-provided BIOS and disc files are in /cd/PSX/.
This diagnostic loads SYSTEM.CNF and the PS-X EXE, then executes PS1 CPU
instructions and sends audio to AICA. GPU presentation and controller input
are not yet integrated; this is not yet a playable emulator.
EOF

    mkdcdisc -e "$BOOT_ELF" -D "$PSX_ROOT" -n "$TITLE PS1 Boot Test" \
        -o "$OUT_DIR/dreampsx-ps1-boot-smoke.cdi"
    [ -s "$OUT_DIR/dreampsx-ps1-boot-smoke.cdi" ] || {
        echo "ERROR: PS1 boot CDI output was not created" >&2
        exit 1
    }
    echo "Created: $OUT_DIR/dreampsx-ps1-boot-smoke.cdi"
else
    echo "No DREAMPSX_PSX_DIR supplied; personalized PS1 boot CDI skipped."
fi
