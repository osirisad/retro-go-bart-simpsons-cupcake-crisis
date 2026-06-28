#!/usr/bin/env bash
# Build release/cupcake.bin from the port repo (ABI — no firmware checkout).
#
# Usage:
#   bash tools/build_cupcake_bin.sh

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAKE="${MAKE:-make}"

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  echo "arm-none-eabi-gcc not found." >&2
  echo "MSYS2 MINGW64: pacman -S mingw-w64-x86_64-arm-none-eabi-gcc mingw-w64-x86_64-arm-none-eabi-binutils" >&2
  exit 1
fi

cd "$ROOT"
"$MAKE" cupcake-bin

echo ""
echo "Done: $ROOT/release/cupcake.bin"
echo "Copy to SD: /roms/homebrew/cupcake.bin"
