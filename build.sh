#!/usr/bin/env bash
# MSYS2 MINGW64: GNU make is installed as mingw32-make, not make.
set -e
cd "$(dirname "$0")"
exec mingw32-make "$@"
