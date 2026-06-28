# Overlay RAM budget (OV-02)

Cupcake links into the same RAM emulator window as Celeste, NES cores, etc. On the **SD-card linker** (`STM32H7B0VBTx_SDCARD.ld`):

| Symbol | Value |
|--------|-------|
| `__RAM_EMU_START__` | `0x2404B000` |
| `__RAM_EMU_LENGTH__` | **724 KiB** (1024 KiB − 300 KiB framebuffer UC) |
| Hard limit | Linker `ASSERT` on `_OVERLAY_CUPCAKE_BSS_END` vs `__RAM_EMU_END__` |

Usage = **load image** (`.overlay_cupcake`: `.text` + `.data` + `.rodata`) + **BSS** (`.overlay_cupcake_bss`). The launcher copies the load image from SD; BSS is zeroed at runtime.

---

## How to measure (authoritative)

After linking firmware:

```bash
cd game-and-watch-retro-go-sd-cupcake
make   # or your usual target
./scripts/size.sh build/<TARGET>.elf
```

Look for the `ram_emu_cupcake` line. Compare to `__RAM_EMU_LENGTH__`.

Extract size of the SD drop-in alone:

```bash
ls -l build/homebrews/cupcake.bin   # path may vary; see HOMEBREWS_FOLDER in Makefile.common
```

`cupcake.bin` size equals `_OVERLAY_CUPCAKE_SIZE` (load image only — BSS is not in the file).

### Stub baseline (2025-06)

Firmware builds with in-tree `Core/Src/porting/cupcake/main_cupcake.c` only. Expect **well under 1 KiB** load image — confirms linker + objcopy + launcher path before the full port lands.

---

## Estimate: full port (pre-link, source audit)

Run when assets exist locally:

```bash
python tools/overlay_size_estimate.py
python tools/overlay_size_estimate.py --assets /path/to/assets
```

Figures below were captured from this repo without committed PNG/JPG/WAV (assets are gitignored).

### Code + generated headers (rodata-heavy)

| Component | Size (bytes) | Notes |
|-----------|-------------:|-------|
| `src/*.c` + `platform/host_*.c` + `cupcake_input.c` | ~112 000 | Source; ARM `.text` typically 1.2–2.5× |
| `cupcake_sprite_masks.h` | **~687 000** | Dominates today — triangle masks for masked blit |
| `cupcake_demo.h` | ~20 000 | Demo TAS frame lists |
| Other generated headers | ~25 000 | sprites, LCD positions, scoreboard tables |
| **Subtotal (headers + source)** | **~844 000** | Already **exceeds 724 KiB** before atlas/audio |

### Embedded assets (measured from local `assets/`)

| Asset | On-disk size | Notes |
|-------|-------------:|-------|
| `sprites-color.png` | ~731 KiB | Embed as PNG bytes; decode at init — not 4 MiB RGBA in rodata |
| `screen.jpg` | ~177 KiB | Same — JPEG in rodata, decode once |
| `assets/audio/*.wav` (17 files) | ~1.25 MiB | Largest single block; consider 22050 Hz mono trim or pack |
| Runtime LCD buffer | 1024×800×4 ≈ **3.1 MiB** | **Must not live in overlay BSS** — use 320×240 RGB565 (~150 KiB) |

Raw PNG + WAV alone (~2 MiB on disk) plus masks (~671 KiB) plus code exceeds 724 KiB if everything lands in `.overlay_cupcake` verbatim. **Asset bundling must store compressed files and/or trim audio**; masks need compression or heap decode (see Verdict).

### Celeste reference

| Item | Size |
|------|-----|
| `celeste_data.h` (font/tiles in firmware tree) | ~115 KiB source file |
| Full Celeste overlay (code + data + BSS) | Run `size.sh` → `ram_emu_celeste` on linked ELF |

Celeste uses a tiny P8 framebuffer and embedded tile/font data — much smaller art than a 1024² atlas.

---

## Verdict (OV-02)

| Question | Answer |
|----------|--------|
| Does stub fit? | **Yes** — trivially |
| Does naïve full port fit? | **No** — masks alone ~687 KiB + code + audio leaves no room for raw atlas/bezel |
| Compression required? | **Yes**, for masks and/or art |

### Recommended plan (no firmware change)

1. **OV-08 asset bundle** — embed **JPEG/PNG bytes** in rodata (not decoded RGBA). One-time decode at `app_main_cupcake` init into **firmware TLSF heap** (after overlay BSS), not into the overlay section.

2. **Masks (OV-08 or OV-07 follow-up)** — pick one:
   - **A.** LZ4/bitpack `cupcake_sprite_masks.h` → decompress to heap at init (~687 KiB → often 80–150 KiB packed; measure with real tool).
   - **B.** Device-only bbox blit (drop masks) — smaller rodata, possible visual regressions on overlapping UVs.
   - **C.** Runtime mask raster from atlas once — CPU cost at boot, saves rodata.

3. **Runtime buffers** — match linux emu: **320×240 RGB565** framebuffer + scaled LCD rect; avoid full 1024×800 RGBA in overlay BSS.

4. **Re-measure** after `make cupcake-bin` — recorded load **148 KiB** + BSS **153 KiB** (2026-06-27).

### When to escalate (firmware issue)

Only if, after compression + heap decode, overlay load image + BSS still overflow `__RAM_EMU`. Alternatives then: shrink art, split data across SD (breaks pure-overlay model), or revisit GWHB multi-segment loading.

---

## Tracking

Update the **Measured** table when `cupcake.bin` from the port repo first links:

| Build | Load image | BSS | Total | Free vs 724 KiB |
|-------|----------:|----:|------:|----------------:|
| Firmware stub | _TBD_ | _TBD_ | _TBD_ | _TBD_ |
| Port full overlay | _TBD_ | _TBD_ | _TBD_ | _TBD_ |
