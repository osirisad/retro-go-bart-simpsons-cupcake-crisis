#!/usr/bin/env python3
"""Rough GNW asset budget estimates for device-native sprites vs audio."""
import math
import re
from pathlib import Path

PORT = Path(__file__).resolve().parents[1]
sprites = []
for line in (PORT / "src" / "cupcake_sprites.h").read_text(encoding="utf-8").splitlines():
    m = re.match(r'\s*\{\s*"([^"]+)",\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\s*\}', line)
    if m:
        sprites.append((m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))))

FB_W, FB_H = 320, 240
LCD_W, LCD_H = 1024, 800
total_area = 0
max_w = max_h = 0
for name, x, y, w, h in sprites:
    dw = (w * FB_W + LCD_W - 1) // LCD_W
    dh = (h * FB_H + LCD_H - 1) // LCD_H
    total_area += dw * dh
    max_w = max(max_w, dw)
    max_h = max(max_h, dh)

side = int(math.ceil(math.sqrt(total_area)))
print(f"sprites: {len(sprites)}")
print(f"device-sized pixel sum: {total_area} ({total_area * 2} B RGB565)")
print(f"largest on device: {max_w}x{max_h}")
print(f"naive pack ~{side}x{side} = {side * side * 2} B")

c = (PORT / "platform" / "gnw" / "cupcake_data.h").read_text(encoding="utf-8")
m = re.search(r"CUPCAKE_GNW_ASSETS_DAT_BYTES (\d+)", c)
m2 = re.search(r"CUPCAKE_GNW_ASSETS_DAT_CLIPS (\d+)", c)
if m and m2:
    print(f"SD audio (cupcake_assets.dat): {m.group(1)} B, {m2.group(1)} clips — not embedded in cupcake.bin")
else:
    print("audio: see cupcake_data.h for assets.dat stats")
