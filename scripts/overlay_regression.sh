#!/usr/bin/env bash
# OV-03 — regression gate for shared overlay / PC / linux-emu code paths.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "== overlay regression: PC unit tests =="
make test-overlay-regression-pc

if [[ "${SKIP_LINUX_EMU:-}" == "1" ]]; then
  echo "== SKIP_LINUX_EMU=1 — skipping Makefile.cupcake =="
  exit 0
fi

FW_ROOT="${RETROGO_FW:-}"
if [[ -z "$FW_ROOT" ]]; then
  # Sibling checkout in monorepo layout
  if [[ -d "$ROOT/../game-and-watch-retro-go-sd-cupcake/linux" ]]; then
    FW_ROOT="$ROOT/../game-and-watch-retro-go-sd-cupcake"
  fi
fi

if [[ -z "$FW_ROOT" || ! -d "$FW_ROOT/linux" ]]; then
  echo "== linux emu: skipped (set RETROGO_FW=path/to/game-and-watch-retro-go-sd-cupcake) =="
  exit 0
fi

echo "== overlay regression: linux emu (Makefile.cupcake) =="
export CUPCAKE_PORT="$ROOT"
make -C "$FW_ROOT/linux" -f "$ROOT/platform/retrogo/Makefile.cupcake" clean all
echo "== linux emu build OK: $FW_ROOT/linux/build-cupcake/retro-go-cupcake.elf =="
