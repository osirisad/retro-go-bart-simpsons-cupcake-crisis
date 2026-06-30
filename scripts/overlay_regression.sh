#!/usr/bin/env bash
# OV-03 — regression gate for shared overlay / PC / linux-emu code paths.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

find_make() {
  if [[ -n "${MAKE:-}" ]] && command -v "$MAKE" >/dev/null 2>&1; then
    command -v "$MAKE"
    return
  fi
  if command -v mingw32-make >/dev/null 2>&1; then
    command -v mingw32-make
    return
  fi
  if command -v make >/dev/null 2>&1; then
    command -v make
    return
  fi
  for candidate in \
    /mingw64/bin/mingw32-make \
    /ucrt64/bin/mingw32-make \
    /c/msys64/mingw64/bin/mingw32-make.exe \
    /c/msys64/ucrt64/bin/mingw32-make.exe; do
    if [[ -x "$candidate" ]]; then
      echo "$candidate"
      return
    fi
  done
  return 1
}

MAKE_BIN="$(find_make)" || {
  echo "overlay_regression.sh: need mingw32-make or make on PATH (or set MAKE=...)" >&2
  exit 127
}

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

ODROID_H="$FW_ROOT/retro-go-stm32/components/odroid/odroid_system.h"
if [[ ! -f "$ODROID_H" ]]; then
  echo "== linux emu: skipped — missing retro-go-stm32 submodule ==" >&2
  echo "   In the firmware repo run:" >&2
  echo "     git submodule update --init retro-go-stm32" >&2
  echo "   Or set SKIP_LINUX_EMU=1 to skip this step." >&2
  exit 0
fi

echo "== overlay regression: linux emu (Makefile.cupcake) =="
export CUPCAKE_PORT="$ROOT"
"$MAKE_BIN" -C "$FW_ROOT/linux" -f "$ROOT/platform/retrogo/Makefile.cupcake" clean all
echo "== linux emu build OK: $FW_ROOT/linux/build-cupcake/retro-go-cupcake.elf =="
