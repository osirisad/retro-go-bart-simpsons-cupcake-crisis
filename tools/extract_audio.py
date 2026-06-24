#!/usr/bin/env python3
"""Extract Cupcake Crisis SFX from an itch.io HAR into assets/audio/.

Original files match game.model (15× MP3, 2× WAV). Optional --wav runs ffmpeg
to build 44.1 kHz stereo PCM .wav files for the SDL host (Mix_LoadWAV).

Default HAR path: repo-root html-classic.itch.zone_Archive *.har
Output: assets/audio/ (gitignored; see docs/AUDIO.md)
"""
from __future__ import annotations

import argparse
import base64
import json
import os
import re
import shutil
import subprocess
import sys
from urllib.parse import unquote, urlparse

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

# game.model sounds block — id -> relative file under game/
SFX_FILES = {
    "bonus": "audio/bonus.mp3",
    "couch": "audio/couch.mp3",
    "cupcake": "audio/cupcake.mp3",
    "deliver": "audio/deliver.mp3",
    "five": "audio/five.mp3",
    "marge": "audio/marge.mp3",
    "miss": "audio/miss.mp3",
    "move": "audio/move.mp3",
    "over": "audio/over.mp3",
    "pacifier1": "audio/pacifier1.mp3",
    "pacifier2": "audio/pacifier2.mp3",
    "phase": "audio/phase.mp3",
    "points": "audio/points.wav",
    "start": "audio/start.mp3",
    "step": "audio/step.wav",
    "throw": "audio/throw.mp3",
    "whoa": "audio/whoa.mp3",
}


def default_har() -> str | None:
    parent = os.path.dirname(ROOT)
    for name in os.listdir(parent):
        if name.endswith(".har") and "itch.zone" in name:
            return os.path.join(parent, name)
    return None


def har_rel_path(url: str) -> str | None:
    m = re.search(r"/html/\d+/(.*)", url)
    rel = m.group(1) if m else urlparse(url).path.lstrip("/")
    return unquote(rel.split("?")[0])


def extract_from_har(har_path: str, out_dir: str) -> dict[str, str]:
    with open(har_path, encoding="utf-8") as f:
        har = json.load(f)

    want = {v.replace("\\", "/") for v in SFX_FILES.values()}
    saved: dict[str, str] = {}

    for entry in har["log"]["entries"]:
        url = entry["request"]["url"]
        rel = har_rel_path(url)
        if not rel:
            continue
        norm = rel.replace("\\", "/")
        if norm not in want:
            continue
        content = entry["response"].get("content", {})
        text = content.get("text", "")
        if not text:
            continue
        data = (
            base64.b64decode(text)
            if content.get("encoding") == "base64"
            else text.encode("utf-8")
        )
        base = os.path.basename(norm)
        path = os.path.join(out_dir, base)
        with open(path, "wb") as f:
            f.write(data)
        saved[base] = path

    return saved


def copy_from_har_extract(src_root: str, out_dir: str) -> dict[str, str]:
    saved: dict[str, str] = {}
    for rel in SFX_FILES.values():
        base = os.path.basename(rel)
        src = os.path.join(src_root, "sim-acclaim-cupcakecrisis", "game", rel.replace("/", os.sep))
        if not os.path.isfile(src):
            continue
        dst = os.path.join(out_dir, base)
        shutil.copy2(src, dst)
        saved[base] = dst
    return saved


def ffmpeg_wav(src: str, dst: str, sample_rate: int) -> bool:
    tmp = dst + ".tmp.wav"
    try:
        subprocess.run(
            [
                "ffmpeg",
                "-y",
                "-loglevel",
                "error",
                "-i",
                src,
                "-ac",
                "1",
                "-ar",
                str(sample_rate),
                "-sample_fmt",
                "s16",
                tmp,
            ],
            check=True,
        )
        os.replace(tmp, dst)
        return True
    except (OSError, subprocess.CalledProcessError):
        if os.path.isfile(tmp):
            os.remove(tmp)
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "har",
        nargs="?",
        default=default_har(),
        help="Path to .har capture (default: repo-root *.har)",
    )
    parser.add_argument(
        "-o",
        "--out",
        default=os.path.join(ROOT, "assets", "audio"),
        help="Output directory (default: assets/audio)",
    )
    parser.add_argument(
        "--har-extracted",
        default=os.path.join(ROOT, "ignore", "har_extracted"),
        help="Fallback tree from tools/extract_har.py",
    )
    parser.add_argument(
        "--wav",
        action="store_true",
        help="Also write id.wav via ffmpeg (needed for SDL Mix_LoadWAV on MP3)",
    )
    parser.add_argument(
        "--rate",
        type=int,
        default=44100,
        help="Sample rate for id.wav output (default 44100; use 22050 for device)",
    )
    args = parser.parse_args()

    out_dir = os.path.abspath(args.out)
    os.makedirs(out_dir, exist_ok=True)

    saved: dict[str, str] = {}
    if args.har and os.path.isfile(args.har):
        print(f"Extracting audio from HAR: {args.har}")
        saved = extract_from_har(args.har, out_dir)
    if len(saved) < len(SFX_FILES) and os.path.isdir(args.har_extracted):
        print(f"Supplementing from {args.har_extracted}")
        saved.update(copy_from_har_extract(args.har_extracted, out_dir))

    missing = []
    for sfx_id, rel in SFX_FILES.items():
        base = os.path.basename(rel)
        if base not in saved and not os.path.isfile(os.path.join(out_dir, base)):
            missing.append(sfx_id)

    if missing:
        print("Missing SFX:", ", ".join(missing), file=sys.stderr)
        print(
            "Run: python tools/extract_har.py path/to/capture.har\n"
            "  or pass a HAR that includes sim-acclaim-cupcakecrisis/game/audio/*",
            file=sys.stderr,
        )
        return 1

    print(f"Shipped {len(SFX_FILES)} files to {out_dir}")

    do_wav = args.wav or bool(shutil.which("ffmpeg"))
    if do_wav:
        if not shutil.which("ffmpeg"):
            print("Tip: pacman -S ffmpeg then re-run with --wav for MP3 playback on PC", file=sys.stderr)
        else:
            for sfx_id, rel in SFX_FILES.items():
                src = os.path.join(out_dir, os.path.basename(rel))
                dst = os.path.join(out_dir, f"{sfx_id}.wav")
                if not ffmpeg_wav(src, dst, args.rate):
                    print(f"ffmpeg failed: {src} -> {dst}", file=sys.stderr)
                    return 1
            print(f"Wrote {len(SFX_FILES)} playback WAV files (id.wav @ {args.rate} Hz)")
    elif any(SFX_FILES[k].endswith(".mp3") for k in SFX_FILES):
        print("Tip: re-run with --wav (needs ffmpeg) so the PC host can play MP3 SFX", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
