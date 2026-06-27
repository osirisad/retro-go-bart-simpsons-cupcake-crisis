#!/usr/bin/env bash
# MSYS2 MINGW64: GNU make is installed as mingw32-make, not make.
# Usage: ./build.sh [target...]   e.g. ./build.sh test-overlay-regression
set -euo pipefail
cd "$(dirname "$0")"

find_make() {
  if command -v mingw32-make >/dev/null 2>&1; then
    command -v mingw32-make
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

MAKE_BIN="${MAKE:-}"
if [[ -z "$MAKE_BIN" ]]; then
  MAKE_BIN="$(find_make)" || true
fi

if [[ -z "$MAKE_BIN" ]]; then
  cat >&2 <<'EOF'
build.sh: mingw32-make not found.

Open **MSYS2 MINGW64** from the Start menu (not plain MSYS / PowerShell), then install tools once:

  pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-make

Then run:

  ./build.sh test-overlay-regression

Optional: alias make=mingw32-make  (add to ~/.bashrc)
EOF
  exit 127
fi

exec "$MAKE_BIN" "$@"
