#!/usr/bin/env bash
# Verify CUPCAKE.bin starts with GWHB magic (0x42485747) and has a thumb entry at +4.
set -euo pipefail

bin="${1:-build-gwhb/CUPCAKE.bin}"

if [[ ! -f "$bin" ]]; then
  echo "verify_gwhb_header: missing $bin" >&2
  exit 1
fi

magic=$(od -An -tx1 -N4 "$bin" | tr -d ' \n')
if [[ "$magic" != "47485742" ]]; then
  echo "verify_gwhb_header: bad magic (want 47485742 GWHB LE, got $magic)" >&2
  exit 1
fi

entry=$(od -An -tx4 -j4 -N4 "$bin" | tr -d ' ')
entry_dec=$((16#${entry#0x}))

if (( entry_dec == 0 )); then
  echo "verify_gwhb_header: null entry at offset 4" >&2
  exit 1
fi

if (( (entry_dec & 1) == 0 )); then
  echo "verify_gwhb_header: entry $entry should be thumb (LSB set)" >&2
  exit 1
fi

echo "verify_gwhb_header: OK magic=GWHB entry=$entry"
