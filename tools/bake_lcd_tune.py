#!/usr/bin/env python3
"""Bake assets/lcd_tune.txt into src/cupcake/cupcake_sprite_lcd.h (no PIL required)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HDR = ROOT / "src" / "cupcake" / "cupcake_sprite_lcd.h"
TUNE = ROOT / "assets" / "lcd_tune.txt"


def load_tune() -> dict[str, tuple[int, int]]:
    out: dict[str, tuple[int, int]] = {}
    for raw in TUNE.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) >= 3:
            out[parts[0]] = (int(parts[1]), int(parts[2]))
    return out


def load_header() -> dict[str, tuple[int, int]]:
    text = HDR.read_text(encoding="utf-8")
    return {
        m.group(1): (int(m.group(2)), int(m.group(3)))
        for m in re.finditer(r'\{ "([^"]+)", (\d+), (\d+) \}', text)
    }


def main() -> int:
    if not TUNE.is_file():
        print(f"Missing {TUNE}", file=sys.stderr)
        return 1
    if not HDR.is_file():
        print(f"Missing {HDR}", file=sys.stderr)
        return 1

    tune = load_tune()
    merged = {**load_header(), **tune}
    changed = [n for n in sorted(tune) if load_header().get(n) != tune[n]]

    preamble = HDR.read_text(encoding="utf-8").split(
        "static const cupcake_sprite_lcd_t cupcake_sprite_lcd[] = {"
    )[0]

    lines = [
        preamble.rstrip(),
        "static const cupcake_sprite_lcd_t cupcake_sprite_lcd[] = {",
    ]
    for name in sorted(merged):
        x, y = merged[name]
        lines.append(f'    {{ "{name}", {x}, {y} }},')
    lines += [
        "};",
        "",
        f"#define CUPCAKE_SPRITE_LCD_COUNT {len(merged)}",
        "",
        "static inline const cupcake_sprite_lcd_t *cupcake_sprite_lcd_by_name(const char *name) {",
        "    for (unsigned i = 0; i < CUPCAKE_SPRITE_LCD_COUNT; i++) {",
        "        const char *a = cupcake_sprite_lcd[i].name, *b = name;",
        "        while (*a && *b && *a == *b) { a++; b++; }",
        "        if (!*a && !*b) return &cupcake_sprite_lcd[i];",
        "    }",
        "    return 0;",
        "}",
        "",
        "#endif",
        "",
    ]
    HDR.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"Baked {len(tune)} tune entries into {HDR} ({len(merged)} total, {len(changed)} changed)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
