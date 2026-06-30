#!/usr/bin/env python3
"""Estimate Cupcake overlay RAM usage vs __RAM_EMU (724 KiB SD layout). OV-02 helper."""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

RAM_EMU_KIB = 724
RAM_EMU_BYTES = RAM_EMU_KIB * 1024

PORT_ROOT = Path(__file__).resolve().parents[1]
SRC = PORT_ROOT / "src"
PLATFORM = PORT_ROOT / "platform"


def fmt(n: int) -> str:
    if n >= 1024 * 1024:
        return f"{n / (1024 * 1024):.2f} MiB"
    if n >= 1024:
        return f"{n / 1024:.1f} KiB"
    return f"{n} B"


def dir_size(path: Path, pattern: str = "*") -> int:
    if not path.is_dir():
        return 0
    total = 0
    for p in path.glob(pattern):
        if p.is_file():
            total += p.stat().st_size
    return total


def file_size(path: Path) -> int:
    return path.stat().st_size if path.is_file() else 0


def estimate_code_sources() -> int:
    total = 0
    for sub in (SRC, PLATFORM):
        if not sub.is_dir():
            continue
        for p in sub.rglob("*.c"):
            if "stb" in p.parts or "sdl" in p.parts or "gwhb" in p.parts:
                continue
            if p.name == "main_cupcake.c":
                total += file_size(p)
                continue
            if p.name.startswith("host_") or p.name == "cupcake_input.c":
                total += file_size(p)
            elif p.parent == SRC:
                total += file_size(p)
    return total


def estimate_headers() -> dict[str, int]:
    out: dict[str, int] = {}
    if not SRC.is_dir():
        return out
    for p in SRC.glob("cupcake_*.h"):
        out[p.name] = file_size(p)
    return out


def estimate_assets(assets_dir: Path) -> dict[str, int]:
    out: dict[str, int] = {}
    if not assets_dir.is_dir():
        return out

    for name in ("sprites-color.png", "sprites.png", "screen.jpg"):
        out[name] = file_size(assets_dir / name)

    out["audio/"] = dir_size(assets_dir / "audio")
    out["audio/*.wav (count)"] = len(list((assets_dir / "audio").glob("*.wav"))) if (assets_dir / "audio").is_dir() else 0

    # Theoretical decoded sizes (would NOT all fit in overlay if stored raw)
    w, h = 1024, 1024
    out["[hypothetical] sprites-color RGBA"] = w * h * 4
    out["[hypothetical] bezel RGBA 1024x800"] = 1024 * 800 * 4
    out["[hypothetical] LCD buffer RGBA"] = 1024 * 800 * 4
    out["[hypothetical] fb 320x240 RGB565"] = 320 * 240 * 2
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--assets",
        type=Path,
        default=PORT_ROOT / "assets",
        help="assets directory (default: port repo assets/)",
    )
    args = ap.parse_args()

    code_src = estimate_code_sources()
    headers = estimate_headers()
    header_total = sum(headers.values())
    assets = estimate_assets(args.assets)

    # Rough ARM thumb estimate for .text (not measured — spike only)
    code_est_low = int(code_src * 1.2)
    code_est_high = int(code_src * 2.5)

    print(f"Overlay RAM slot (SD layout): {RAM_EMU_KIB} KiB ({RAM_EMU_BYTES} bytes)\n")

    print("=== Source (device-relevant .c) ===")
    print(f"  {fmt(code_src)}  (ARM .text estimate: {fmt(code_est_low)} – {fmt(code_est_high)})")

    print("\n=== Generated headers (linked .rodata) ===")
    for name in sorted(headers, key=lambda k: headers[k], reverse=True):
        print(f"  {name:28} {fmt(headers[name])}")
    print(f"  {'TOTAL':28} {fmt(header_total)}")

    rodata_spike = header_total + code_est_low
    print(f"\n  Spike subtotal (headers + low code est): {fmt(rodata_spike)}", end="")
    if rodata_spike > RAM_EMU_BYTES:
        print(f"  *** OVER {fmt(RAM_EMU_BYTES - rodata_spike)} ***")
    else:
        print(f"  ({fmt(RAM_EMU_BYTES - rodata_spike)} headroom before assets/BSS)")

    print("\n=== Assets directory ===")
    if not args.assets.is_dir():
        print(f"  (missing {args.assets} — run extract pipeline first)")
    else:
        for key in sorted(assets):
            val = assets[key]
            if key.endswith("(count)"):
                print(f"  {key:36} {val}")
            else:
                print(f"  {key:36} {fmt(val)}")

    masks = headers.get("cupcake_sprite_masks.h", 0)
    print("\n=== OV-02 notes ===")
    if masks > RAM_EMU_BYTES * 0.8:
        print("  • cupcake_sprite_masks.h alone is most of the overlay — compress or decode-to-heap (see OVERLAY_RAM_BUDGET.md).")
    atlas = assets.get("[hypothetical] sprites-color RGBA", 0)
    if atlas > RAM_EMU_BYTES:
        print("  • Do not embed full 1024² RGBA atlas in .rodata — store PNG/JPEG and decode at init.")
    print("  • Authoritative size: firmware scripts/size.sh on linked ELF (ram_emu_cupcake).")

    return 0


if __name__ == "__main__":
    sys.exit(main())
