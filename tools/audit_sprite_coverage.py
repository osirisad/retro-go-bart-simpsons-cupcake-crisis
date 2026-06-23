#!/usr/bin/env python3
"""Compare sprites.json materials vs generated cupcake_sprites.h (TASK-46)."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPRITES_JSON = ROOT / "ignore/har_extracted/sim-acclaim-cupcakecrisis/game/sprites.json"
SPR_HDR = ROOT / "src/cupcake_sprites.h"

# Materials in JSON that are not gameplay atlas sprites.
EXCLUDE = {
    "sprites",  # atlas material name
    "TEMP4",
    "digit02",  # 0 visible atlas pixels (unused middle segment for digit 0)
    "digit12",  # 0 visible atlas pixels (unused middle segment for digit 1)
}


def main() -> int:
    data = json.loads(SPRITES_JSON.read_text(encoding="utf-8"))
    json_names = sorted(
        m["DbgName"] for m in data["materials"] if m.get("DbgName") and m["DbgName"] not in EXCLUDE
    )
    hdr_names = sorted(
        m.group(1)
        for m in re.finditer(r'\{ "([^"]+)",', SPR_HDR.read_text(encoding="utf-8"))
    )
    missing = [n for n in json_names if n not in hdr_names]
    extra = [n for n in hdr_names if n not in json_names]

    print(f"JSON materials (excl. {len(EXCLUDE)}): {len(json_names)}")
    print(f"Generated headers: {len(hdr_names)}")
    if missing:
        print("\nMissing from headers:")
        for n in missing:
            print(f"  {n}")
    if extra:
        print("\nExtra in headers:")
        for n in extra:
            print(f"  {n}")

    ok = not missing
    print("\nPASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
