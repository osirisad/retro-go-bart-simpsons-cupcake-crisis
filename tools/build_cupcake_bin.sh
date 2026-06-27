#!/usr/bin/env bash
# Build release/cupcake.bin from the port repo (standalone — no firmware source required
# once platform/gnw/firmware_imports.ld is committed).
#
# Usage:
#   bash tools/build_cupcake_bin.sh
#   FIRMWARE_ELF=/path/to/gw_retro_go.elf bash tools/build_cupcake_bin.sh
#
# One-time: generate imports from the firmware you flash on device:
#   bash tools/gen_overlay_imports.sh /path/to/gw_retro_go.elf > platform/gnw/firmware_imports.ld

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMPORTS="${ROOT}/platform/gnw/firmware_imports.ld"
MAKE="${MAKE:-make}"

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  echo "arm-none-eabi-gcc not found." >&2
  echo "MSYS2 MINGW64: pacman -S mingw-w64-x86_64-arm-none-eabi-gcc mingw-w64-x86_64-arm-none-eabi-binutils" >&2
  exit 1
fi

if [ ! -f "$IMPORTS" ] && [ -z "${FIRMWARE_ELF:-}" ]; then
  echo "Missing $IMPORTS" >&2
  echo "Build matching firmware once, then:" >&2
  echo "  bash tools/gen_overlay_imports.sh /path/to/gw_retro_go.elf > platform/gnw/firmware_imports.ld" >&2
  exit 1
fi

cd "$ROOT"
if [ -n "${FIRMWARE_ELF:-}" ]; then
  "$MAKE" -f platform/gnw/Makefile.gnw FIRMWARE_ELF="$FIRMWARE_ELF"
else
  "$MAKE" -f platform/gnw/Makefile.gnw
fi

echo ""
echo "Done: $ROOT/release/cupcake.bin"
echo "Copy to SD: /roms/homebrew/cupcake.bin"
