#!/usr/bin/env python3
"""Regenerate assets/lcd_tune.txt layout from cupcake_sprite_lcd.h (preserves grouping)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HDR = ROOT / "src" / "cupcake" / "cupcake_sprite_lcd.h"
OUT = ROOT / "assets" / "lcd_tune.txt"

# (group header, [(comment, sprite_name), ...])
GROUPS: list[tuple[str, list[tuple[str, str]]]] = [
    (
        "Bart (bart0..bart9)",
        [
            ("throw / hand-off (pos 0)", "bart0"),
            ("lane 1", "bart1"),
            ("lane 2", "bart2"),
            ("lane 3", "bart3"),
            ("lane 4", "bart4"),
            ("couch sitting (pos 5)", "bart5"),
            ("couch sit miss", "bart6"),
            ("lane 1 plate / catch layer", "bart7"),
            ("couch plate layer (pos 5)", "bart8"),
            ("cupcake miss", "bart9"),
        ],
    ),
    (
        "Maggie (maggie0..maggie3)",
        [
            ("base (always visible when Maggie on screen)", "maggie0"),
            ("throw overlay loop 1", "maggie1"),
            ("throw overlay loop 2", "maggie2"),
            ("throw overlay loop 3", "maggie3"),
        ],
    ),
    (
        "Marge",
        [("", "marge1")],
    ),
    (
        "Pacifier",
        [
            ("", "pacifier1"),
            ("", "pacifier2"),
        ],
    ),
    (
        "Couch (couch0..couch4)",
        [
            ("spawn frame 0", "couch0"),
            ("spawn frame 1 (speech bubble)", "couch1"),
            ("spawn frame 2", "couch2"),
            ("sit target / bonus frame 3", "couch3"),
            ("spawn frame 4", "couch4"),
        ],
    ),
    (
        "Miss counter (top-left)",
        [
            ("", "miss1"),
            ("", "miss2"),
            ("", "miss3"),
        ],
    ),
    (
        "Grid cupcakes — lane 1 (cake01..cake05, slots 1-5)",
        [(f"slot {i}", f"cake0{i}") for i in range(1, 6)],
    ),
    (
        "Grid cupcakes — lane 2 (cake11..cake15)",
        [(f"slot {i}", f"cake1{i}") for i in range(1, 6)],
    ),
    (
        "Grid cupcakes — lane 3 (cake21..cake25)",
        [(f"slot {i}", f"cake2{i}") for i in range(1, 6)],
    ),
    (
        "Grid cupcakes — lane 4 (cake31..cake35)",
        [(f"slot {i}", f"cake3{i}") for i in range(1, 6)],
    ),
    (
        "Grid cupcakes — lane 5 / couch row (cake41..cake45)",
        [(f"slot {i}", f"cake4{i}") for i in range(1, 6)],
    ),
    (
        "Flying aircakes (cake0..cake9) — Maggie throw / catch lanes",
        [(f"aircake {i}", f"cake{i}") for i in range(10)],
    ),
]


def load_lcd() -> dict[str, tuple[int, int]]:
    text = HDR.read_text(encoding="utf-8")
    return {
        m.group(1): (int(m.group(2)), int(m.group(3)))
        for m in re.finditer(r'\{ "([^"]+)", (\d+), (\d+) \}', text)
    }


def load_existing_tune() -> dict[str, tuple[int, int]]:
    """Keep current lcd_tune.txt coords when regen layout from header."""
    if not OUT.exists():
        return {}
    out: dict[str, tuple[int, int]] = {}
    for raw in OUT.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) >= 3:
            out[parts[0]] = (int(parts[1]), int(parts[2]))
    return out


def main() -> None:
    hdr = load_lcd()
    existing = load_existing_tune()
    use = {**hdr, **existing}  # existing tune wins over header

    lines = [
        "# Cupcake Crisis — LCD alignment tune file",
        "# Edit lcd_x lcd_y here; save and re-run (no rebuild).",
        "#",
        "#   run-align.bat          couch + grid + aircake alignment scene",
        "#   build-pc\\cupcake-sdl.exe --pin --pin-solo",
        "#",
        "# Regen layout: python tools/gen_lcd_tune_align.py",
        "# Seeds coords from existing lcd_tune.txt, else cupcake_sprite_lcd.h.",
        "# Bake into header: make gen  (tools/gen_lcd_positions.py reads lcd_tune.txt)",
        "",
    ]

    missing: list[str] = []
    count = 0
    for title, entries in GROUPS:
        lines.append(f"# === {title} ===")
        for comment, name in entries:
            if name not in use:
                missing.append(name)
                continue
            x, y = use[name]
            if comment:
                lines.append(f"{name} {x} {y}    # {comment}")
            else:
                lines.append(f"{name} {x} {y}")
            count += 1
        lines.append("")

    if missing:
        print("Warning: not in header/tune:", ", ".join(missing), file=sys.stderr)

    OUT.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    print(f"Wrote {count} sprites to {OUT}")


if __name__ == "__main__":
    main()
