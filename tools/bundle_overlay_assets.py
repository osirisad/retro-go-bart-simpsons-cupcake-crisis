#!/usr/bin/env python3
"""
Bundle PNG/JPG/WAV assets into platform/gnw/cupcake_data.{h,c} for GNW overlay.

Images are re-encoded as JPEG (atlas RGBA composited on black — masks handle alpha).
Audio is packed as mono IMA ADPCM (~4:1 vs 16-bit PCM).

JPEG quality is auto-tuned to fit the overlay RAM budget (load + BSS <= 724 KiB).
"""

from __future__ import annotations

import argparse
import io
import os
import re
import struct
import sys
import wave
from dataclasses import dataclass

try:
    from PIL import Image
except ImportError:
    print("bundle_overlay_assets.py: requires Pillow (pip install pillow)", file=sys.stderr)
    sys.exit(1)

PORT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DEFAULT_ASSETS = os.path.join(PORT_ROOT, "assets")
DEFAULT_OUT_H = os.path.join(PORT_ROOT, "platform", "gnw", "cupcake_data.h")
DEFAULT_OUT_C = os.path.join(PORT_ROOT, "platform", "gnw", "cupcake_data.c")

# Measured baseline from GNW overlay link (code in .overlay_cupcake excluding embed).
DEFAULT_CODE_LOAD = 138000
DEFAULT_BSS = 157120
DEFAULT_RAM_SLOT = 724 * 1024
# All embedded clips at device-native 22.05 kHz.
GNW_SFX_SAMPLE_RATE = 22050
GNW_MUSIC_SAMPLE_RATE = 22050
# Long music streams as individual .adpcm + .meta files on SD (one FILE per voice).
# All gameplay SFX embed in cupcake.bin.
GNW_SD_WAV_FILES = frozenset({
    "start.wav",
    "phase.wav",
    "over.wav",
})
# Leave headroom so link-time BSS does not overflow the 724 KiB slot.
GNW_RAM_SAFETY_MARGIN = 8192
# Scale PCM before ADPCM encode (device only — does not modify assets/audio/*.wav on disk).
GNW_PCM_PACK_GAIN = 0.10

# Device framebuffer + pre-baked RGB565 embed (no runtime JPEG decode on GNW).
GNW_BEZEL_W = 320
GNW_BEZEL_H = 240
GNW_BEZEL_VISIBLE_H = 800
GNW_ATLAS_SOURCE_W = 1024
GNW_ATLAS_SOURCE_H = 1024

STEP_TABLE = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878,
    2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894,
    6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
    18500, 20350, 22385, 24623, 27086, 29794, 32767,
]
INDEX_TABLE = [
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
]

# Parsed from platform/host_audio_catalog.c
CATALOG_RE = re.compile(
    r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*([0-9.]+)f\s*\}'
)


@dataclass
class SfxEntry:
    sfx_id: str
    file: str
    volume: float


@dataclass
class AdpcmBlob:
    file: str
    pcm_samples: int
    sample_rate: int
    payload: bytes


def parse_catalog(catalog_path: str) -> list[SfxEntry]:
    text = open(catalog_path, encoding="utf-8").read()
    out: list[SfxEntry] = []
    for m in CATALOG_RE.finditer(text):
        out.append(SfxEntry(m.group(1), m.group(2), float(m.group(3))))
    if not out:
        raise RuntimeError(f"no SFX entries parsed from {catalog_path}")
    return out


def atlas_to_jpeg_bytes(png_path: str, quality: int) -> bytes:
    im = Image.open(png_path)
    if im.mode == "RGBA":
        bg = Image.new("RGB", im.size, (0, 0, 0))
        bg.paste(im, mask=im.split()[3])
        im = bg
    elif im.mode != "RGB":
        im = im.convert("RGB")
    buf = io.BytesIO()
    im.save(buf, format="JPEG", quality=quality, optimize=True)
    return buf.getvalue()


def screen_to_jpeg_bytes(jpg_path: str, quality: int) -> bytes:
    im = Image.open(jpg_path)
    if im.mode != "RGB":
        im = im.convert("RGB")
    buf = io.BytesIO()
    im.save(buf, format="JPEG", quality=quality, optimize=True)
    return buf.getvalue()


def _rgb888_to_rgb565(r: int, g: int, b: int, alpha: int = 255) -> int:
  # 7-segment / LED art uses very dark red (e.g. RGB 4,3,3) that becomes 0x0000 in RGB565.
    if alpha > 32 and (r + g + b) > 0 and r < 48:
        r = 48
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def _image_to_rgb565_bytes(im: Image.Image, out_w: int, out_h: int) -> bytes:
    im = im.resize((out_w, out_h), Image.Resampling.LANCZOS)
    if im.mode != "RGBA":
        im = im.convert("RGBA")
    out = bytearray(out_w * out_h * 2)
    px = im.load()
    for y in range(out_h):
        for x in range(out_w):
            r, g, b, a = px[x, y]
            if a < 16:
                val = 0
            else:
                val = _rgb888_to_rgb565(r, g, b, a)
            struct.pack_into("<H", out, (y * out_w + x) * 2, val)
    return bytes(out)


def screen_to_gnw_bezel_rgb565(screen_path: str) -> bytes:
    im = Image.open(screen_path)
    if im.height > GNW_BEZEL_VISIBLE_H:
        im = im.crop((0, 0, im.width, GNW_BEZEL_VISIBLE_H))
    return _image_to_rgb565_bytes(im, GNW_BEZEL_W, GNW_BEZEL_H)


def atlas_to_gnw_rgb565(atlas_path: str, out_w: int, out_h: int) -> bytes:
    im = Image.open(atlas_path)
    return _image_to_rgb565_bytes(im, out_w, out_h)


def pick_gnw_rgb565_layout(
    atlas_path: str,
    screen_path: str,
    code_load: int,
    bss: int,
    ram_slot: int,
    audio_bytes: int,
) -> tuple[int, int, bytes, bytes]:
    """Pick the largest atlas that fits; bezel is fixed 320x240 RGB565."""
    bezel = screen_to_gnw_bezel_rgb565(screen_path)
    best: tuple[int, int, bytes, bytes] | None = None
    for atlas_w in range(320, 95, -16):
        for atlas_h in range(320, 95, -16):
            if abs(atlas_w - atlas_h) > 32:
                continue
            atlas = atlas_to_gnw_rgb565(atlas_path, atlas_w, atlas_h)
            embed = len(bezel) + len(atlas) + audio_bytes
            if code_load + embed + bss <= ram_slot - GNW_RAM_SAFETY_MARGIN:
                if best is None or (atlas_w * atlas_h) > (best[0] * best[1]):
                    best = (atlas_w, atlas_h, bezel, atlas)
    if best is None:
        raise RuntimeError(
            f"GNW RGB565 assets do not fit RAM slot ({ram_slot} B); "
            f"code={code_load} bss={bss} audio={audio_bytes}"
        )
    return best


def read_wav_mono_pcm(path: str) -> tuple[list[int], int]:
    with wave.open(path, "rb") as wf:
        channels = wf.getnchannels()
        width = wf.getsampwidth()
        rate = wf.getframerate()
        frames = wf.getnframes()
        if width != 2:
            raise RuntimeError(f"{path}: expected 16-bit WAV, got {width * 8}-bit")
        raw = wf.readframes(frames)
    samples: list[int] = []
    for i in range(0, len(raw), 2):
        s = struct.unpack_from("<h", raw, i)[0]
        samples.append(s)
    if channels > 1:
        mono: list[int] = []
        for i in range(0, len(samples), channels):
            chunk = samples[i : i + channels]
            mono.append(int(sum(chunk) / len(chunk)))
        samples = mono
    return samples, rate


def embed_sample_rate_for(file: str) -> int:
    if file in GNW_SD_WAV_FILES:
        return GNW_MUSIC_SAMPLE_RATE
    return GNW_SFX_SAMPLE_RATE


def _prefilter_for_downsample(samples: list[int]) -> list[int]:
    """Light smoothing before decimation to reduce aliasing on music beds."""
    if len(samples) < 3:
        return samples
    out = [samples[0]]
    for i in range(1, len(samples) - 1):
        out.append((samples[i - 1] + 2 * samples[i] + samples[i + 1]) // 4)
    out.append(samples[-1])
    return out


def resample_pcm(samples: list[int], src_rate: int, dst_rate: int) -> list[int]:
    """Linear resample mono PCM (used to pack SFX at GNW_EMBED_SAMPLE_RATE)."""
    if not samples or src_rate == dst_rate:
        return samples
    if src_rate < 1 or dst_rate < 1:
        return samples

    if dst_rate < src_rate:
        samples = _prefilter_for_downsample(samples)

    out_len = max(1, int(round(len(samples) * dst_rate / src_rate)))
    out: list[int] = []
    for i in range(out_len):
        src_pos = i * src_rate / dst_rate
        idx = int(src_pos)
        frac = src_pos - idx
        if idx >= len(samples) - 1:
            s = samples[-1]
        else:
            a = samples[idx]
            b = samples[idx + 1]
            s = int(a + (b - a) * frac)
        if s > 32767:
            s = 32767
        if s < -32768:
            s = -32768
        out.append(s)
    return out


def adpcm_encode(samples: list[int]) -> bytes:
    if not samples:
        return b"\x00\x00\x00"

    predictor = max(-32768, min(32767, int(samples[0])))
    step_index = 0
    out = bytearray()
    out.extend(struct.pack("<h", predictor))
    out.append(step_index & 0xFF)

    nibble_buf: list[int] = []
    for i in range(1, len(samples)):
        target = max(-32768, min(32767, int(samples[i])))
        step = STEP_TABLE[step_index]
        diff = target - predictor
        nibble = 0
        if diff < 0:
            nibble = 8
            diff = -diff

        v = step >> 3
        if diff >= step:
            nibble |= 4
            diff -= step
        v += step >> 3
        if diff >= (step >> 1):
            nibble |= 2
            diff -= step >> 1
        v += step >> 2
        if diff >= (step >> 2):
            nibble |= 1

        decode_diff = step >> 3
        if nibble & 1:
            decode_diff += step >> 2
        if nibble & 2:
            decode_diff += step >> 1
        if nibble & 4:
            decode_diff += step
        if nibble & 8:
            decode_diff = -decode_diff

        predictor += decode_diff
        if predictor > 32767:
            predictor = 32767
        if predictor < -32768:
            predictor = -32768

        step_index += INDEX_TABLE[nibble]
        if step_index < 0:
            step_index = 0
        if step_index > 88:
            step_index = 88

        nibble_buf.append(nibble)
        if len(nibble_buf) == 2:
            out.append(nibble_buf[0] | (nibble_buf[1] << 4))
            nibble_buf.clear()

    if len(nibble_buf) == 1:
        out.append(nibble_buf[0] | 0x00)

    return bytes(out)


def apply_pcm_pack_gain(samples: list[int]) -> list[int]:
    """Attenuate mono PCM before GNW ADPCM encode."""
    gain = GNW_PCM_PACK_GAIN
    if gain >= 0.999:
        return samples
    if gain <= 0.0:
        return [0] * len(samples)
    out: list[int] = []
    for s in samples:
        v = int(round(s * gain))
        if v > 32767:
            v = 32767
        if v < -32768:
            v = -32768
        out.append(v)
    return out


def encode_wav_file(path: str, file: str, target_rate: int) -> AdpcmBlob:
    samples, rate = read_wav_mono_pcm(path)
    if rate != target_rate:
        samples = resample_pcm(samples, rate, target_rate)
        rate = target_rate
    samples = apply_pcm_pack_gain(samples)
    payload = adpcm_encode(samples)
    return AdpcmBlob(file, len(samples), rate, payload)


def export_sd_adpcm_files(catalog: list[SfxEntry], audio_dir: str, out_dir: str) -> list[AdpcmBlob]:
    """Write raw .adpcm + .meta sidecars for long music tracks on SD."""
    blobs: list[AdpcmBlob] = []
    audio_out = os.path.join(out_dir, "audio")
    os.makedirs(audio_out, exist_ok=True)

    for entry in catalog:
        if entry.file not in GNW_SD_WAV_FILES:
            continue
        path = os.path.join(audio_dir, entry.file)
        if not os.path.isfile(path):
            raise FileNotFoundError(path)
        target_rate = embed_sample_rate_for(entry.file)
        blob = encode_wav_file(path, entry.file, target_rate)
        base = os.path.splitext(blob.file)[0]
        adpcm_path = os.path.join(audio_out, f"{base}.adpcm")
        meta_path = os.path.join(audio_out, f"{base}.meta")
        with open(adpcm_path, "wb") as f:
            f.write(blob.payload)
        with open(meta_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(f"pcm_samples={blob.pcm_samples}\n")
            f.write(f"sample_rate={blob.sample_rate}\n")
        print(
            f"audio-sd: {base}.adpcm + {base}.meta @ {blob.sample_rate} Hz "
            f"({blob.pcm_samples} samples, {len(blob.payload)} B ADPCM, gain={GNW_PCM_PACK_GAIN})",
            file=sys.stderr,
        )
        blobs.append(blob)

    print(f"Wrote {len(blobs)} SD clip pairs to {audio_out}/", file=sys.stderr)
    return blobs


def encode_embed_wav(catalog: list[SfxEntry], audio_dir: str) -> list[AdpcmBlob]:
    """ADPCM blobs linked into cupcake.bin (all SFX — no SD reads at runtime)."""
    blobs: list[AdpcmBlob] = []
    for entry in catalog:
        if entry.file in GNW_SD_WAV_FILES:
            continue
        path = os.path.join(audio_dir, entry.file)
        if not os.path.isfile(path):
            raise FileNotFoundError(path)
        target_rate = embed_sample_rate_for(entry.file)
        blob = encode_wav_file(path, entry.file, target_rate)
        gain = GNW_PCM_PACK_GAIN
        print(
            f"audio-embed: {entry.file} @ {blob.sample_rate} Hz "
            f"({blob.pcm_samples} samples, {len(blob.payload)} B ADPCM, gain={gain})",
            file=sys.stderr,
        )
        blobs.append(blob)
    return blobs


def pick_jpeg_qualities(
    atlas_path: str,
    screen_path: str,
    embed_budget: int,
) -> tuple[int, int, bytes, bytes]:
    best: tuple[int, int, bytes, bytes] | None = None
    for aq in range(75, 19, -5):
        atlas = atlas_to_jpeg_bytes(atlas_path, aq)
        for sq in range(75, 39, -5):
            screen = screen_to_jpeg_bytes(screen_path, sq)
            total = len(atlas) + len(screen)
            if total <= embed_budget:
                if best is None or total > best[2].__len__() + best[3].__len__():
                    best = (aq, sq, atlas, screen)
    if best is None:
        aq, sq = 25, 45
        return aq, sq, atlas_to_jpeg_bytes(atlas_path, aq), screen_to_jpeg_bytes(screen_path, sq)
    return best


def c_bytes_array(name: str, data: bytes, line_width: int = 16) -> str:
    lines = [f"const uint8_t {name}[] = {{"]
    row: list[str] = []
    for i, b in enumerate(data):
        row.append(f"0x{b:02x}")
        if len(row) >= line_width:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    lines.append("};")
    lines.append(f"const uint32_t {name}_size = (uint32_t)sizeof({name});")
    return "\n".join(lines)


def c_uint16_array(name: str, data: bytes, line_width: int = 12) -> str:
    lines = [f"const uint16_t {name}[] = {{"]
    row: list[str] = []
    for i in range(0, len(data), 2):
        val = struct.unpack_from("<H", data, i)[0]
        row.append(f"0x{val:04x}")
        if len(row) >= line_width:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    lines.append("};")
    lines.append(f"const uint32_t {name}_count = (uint32_t)(sizeof({name}) / sizeof({name}[0]));")
    return "\n".join(lines)


def sanitize_sym(s: str) -> str:
    return re.sub(r"[^a-zA-Z0-9_]", "_", s)


def emit_files(
    out_h: str,
    out_c: str,
    atlas_w: int,
    atlas_h: int,
    bezel_rgb565: bytes,
    atlas_rgb565: bytes,
    wav_blobs: list[AdpcmBlob],
    sd_audio_bytes: int,
    sd_audio_clips: int,
    code_load: int,
    bss: int,
    ram_slot: int,
) -> None:
    audio_bytes = sum(len(b.payload) for b in wav_blobs)
    embed_total = len(bezel_rgb565) + len(atlas_rgb565) + audio_bytes
    load_total = code_load + embed_total
    ram_total = load_total + bss

    header = f"""/* Auto-generated by tools/bundle_overlay_assets.py — do not edit. */
#ifndef CUPCAKE_DATA_H_
#define CUPCAKE_DATA_H_

#include <stdint.h>

typedef struct {{
    const char *file;
    const uint8_t *adpcm;
    uint32_t adpcm_size;
    uint32_t pcm_samples;
}} cupcake_embedded_wav_t;

#define CUPCAKE_GNW_BEZEL_W {GNW_BEZEL_W}
#define CUPCAKE_GNW_BEZEL_H {GNW_BEZEL_H}
#define CUPCAKE_GNW_ATLAS_W {atlas_w}
#define CUPCAKE_GNW_ATLAS_H {atlas_h}
#define CUPCAKE_GNW_ATLAS_SOURCE_W {GNW_ATLAS_SOURCE_W}
#define CUPCAKE_GNW_ATLAS_SOURCE_H {GNW_ATLAS_SOURCE_H}
#define CUPCAKE_GNW_SFX_SAMPLE_RATE {GNW_SFX_SAMPLE_RATE}
#define CUPCAKE_GNW_MUSIC_SAMPLE_RATE {GNW_MUSIC_SAMPLE_RATE}
#define CUPCAKE_GNW_PCM_PACK_GAIN {GNW_PCM_PACK_GAIN}
#define CUPCAKE_GNW_SD_AUDIO_BYTES {sd_audio_bytes}
#define CUPCAKE_GNW_SD_AUDIO_CLIPS {sd_audio_clips}
#define CUPCAKE_EMBED_AUDIO_COUNT {len(wav_blobs)}
#define CUPCAKE_EMBED_BYTES {embed_total}
#define CUPCAKE_EMBED_LOAD_ESTIMATE {load_total}
#define CUPCAKE_EMBED_RAM_ESTIMATE {ram_total}

const uint16_t *cupcake_gnw_bezel_rgb565(void);
const uint16_t *cupcake_gnw_atlas_rgb565(void);
int cupcake_gnw_bezel_pixel_count(void);
int cupcake_gnw_atlas_pixel_count(void);
int cupcake_embedded_wav_count(void);
const cupcake_embedded_wav_t *cupcake_embedded_wav_by_file(const char *file);

#endif
"""

    c_parts = [
        "/* Auto-generated by tools/bundle_overlay_assets.py — do not edit. */",
        '#include "cupcake_data.h"',
        "",
        c_uint16_array("cupcake_gnw_bezel_rgb565_data", bezel_rgb565),
        "",
        c_uint16_array("cupcake_gnw_atlas_rgb565_data", atlas_rgb565),
        "",
    ]

    wav_rows: list[str] = []
    for blob in wav_blobs:
        sym = f"cupcake_wav_{sanitize_sym(os.path.splitext(blob.file)[0])}"
        c_parts.append(c_bytes_array(sym, blob.payload))
        c_parts.append("")
        wav_rows.append(
            f'    {{ "{blob.file}", {sym}, {sym}_size, {blob.pcm_samples}u }},'
        )

    c_parts.extend(
        [
            "static const cupcake_embedded_wav_t cupcake_embedded_wavs[] = {",
            *wav_rows,
            "};",
            "",
            "const uint16_t *cupcake_gnw_bezel_rgb565(void)",
            "{",
            "    return cupcake_gnw_bezel_rgb565_data;",
            "}",
            "",
            "const uint16_t *cupcake_gnw_atlas_rgb565(void)",
            "{",
            "    return cupcake_gnw_atlas_rgb565_data;",
            "}",
            "",
            "int cupcake_gnw_bezel_pixel_count(void)",
            "{",
            "    return (int)cupcake_gnw_bezel_rgb565_data_count;",
            "}",
            "",
            "int cupcake_gnw_atlas_pixel_count(void)",
            "{",
            "    return (int)cupcake_gnw_atlas_rgb565_data_count;",
            "}",
            "",
            "int cupcake_embedded_wav_count(void)",
            "{",
            "    return (int)(sizeof cupcake_embedded_wavs / sizeof cupcake_embedded_wavs[0]);",
            "}",
            "",
            "const cupcake_embedded_wav_t *cupcake_embedded_wav_by_file(const char *file)",
            "{",
            "    int i;",
            "    if (!file)",
            "        return 0;",
            "    for (i = 0; i < cupcake_embedded_wav_count(); i++) {",
            "        if (cupcake_embedded_wavs[i].file && cupcake_embedded_wavs[i].file[0]) {",
            "            const char *a = cupcake_embedded_wavs[i].file;",
            "            const char *b = file;",
            "            while (*a && *b && *a == *b) {",
            "                a++;",
            "                b++;",
            "            }",
            "            if (*a == '\\0' && *b == '\\0')",
            "                return &cupcake_embedded_wavs[i];",
            "        }",
            "    }",
            "    return 0;",
            "}",
            "",
        ]
    )

    os.makedirs(os.path.dirname(out_h), exist_ok=True)
    with open(out_h, "w", encoding="utf-8", newline="\n") as f:
        f.write(header)
    with open(out_c, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(c_parts))

    print(f"Wrote {out_h}")
    print(f"Wrote {out_c}")
    print(
        f"Embed: bezel RGB565 {len(bezel_rgb565)} B ({GNW_BEZEL_W}x{GNW_BEZEL_H}), "
        f"atlas RGB565 {len(atlas_rgb565)} B ({atlas_w}x{atlas_h}), "
        f"audio {audio_bytes} B ({len(wav_blobs)} clips in bin)"
    )
    print(
        f"SD audio: {sd_audio_bytes} B ({sd_audio_clips} clip pairs in cupcake_sd/.../audio/ — "
        f"{', '.join(sorted(GNW_SD_WAV_FILES))})"
    )
    print(f"Estimated load {load_total} B + BSS {bss} B = {ram_total} B / {ram_slot} B")
    if ram_total > ram_slot:
        print(
            f"ERROR: estimated RAM {ram_total} exceeds slot {ram_slot} "
            f"by {ram_total - ram_slot} B",
            file=sys.stderr,
        )
        sys.exit(1)


def main() -> None:
    ap = argparse.ArgumentParser(description="Bundle overlay assets for cupcake.bin")
    ap.add_argument("--assets", default=DEFAULT_ASSETS, help="assets/ directory")
    ap.add_argument("--catalog", default=os.path.join(PORT_ROOT, "platform", "host_audio_catalog.c"))
    ap.add_argument("--out-h", default=DEFAULT_OUT_H)
    ap.add_argument("--out-c", default=DEFAULT_OUT_C)
    ap.add_argument("--code-load", type=int, default=DEFAULT_CODE_LOAD)
    ap.add_argument("--bss", type=int, default=DEFAULT_BSS)
    ap.add_argument("--ram-slot", type=int, default=DEFAULT_RAM_SLOT)
    ap.add_argument(
        "--export-sd-audio-dir",
        default=os.path.join(PORT_ROOT, "release", "cupcake_sd", "roms", "homebrew", "cupcake"),
        help="write music .adpcm+.meta (copy audio/ to /roms/homebrew/cupcake/audio/)",
    )
    args = ap.parse_args()

    screen_path = os.path.join(args.assets, "screen.jpg")
    atlas_path = os.path.join(args.assets, "sprites-color.png")
    audio_dir = os.path.join(args.assets, "audio")

    for path in (screen_path, atlas_path):
        if not os.path.isfile(path):
            print(f"bundle_overlay_assets.py: missing {path}", file=sys.stderr)
            sys.exit(1)
    if not os.path.isdir(audio_dir):
        print(f"bundle_overlay_assets.py: missing {audio_dir}", file=sys.stderr)
        sys.exit(1)

    catalog = parse_catalog(args.catalog)
    embed_blobs = encode_embed_wav(catalog, audio_dir)
    embed_audio_bytes = sum(len(b.payload) for b in embed_blobs)
    sd_blobs = export_sd_adpcm_files(catalog, audio_dir, args.export_sd_audio_dir)
    sd_audio_bytes = sum(len(b.payload) for b in sd_blobs)

    atlas_w, atlas_h, bezel_rgb565, atlas_rgb565 = pick_gnw_rgb565_layout(
        atlas_path,
        screen_path,
        args.code_load,
        args.bss,
        args.ram_slot,
        embed_audio_bytes,
    )
    emit_files(
        args.out_h,
        args.out_c,
        atlas_w,
        atlas_h,
        bezel_rgb565,
        atlas_rgb565,
        embed_blobs,
        sd_audio_bytes,
        len(sd_blobs),
        args.code_load,
        args.bss,
        args.ram_slot,
    )


if __name__ == "__main__":
    main()
