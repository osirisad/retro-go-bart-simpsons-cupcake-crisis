# Cupcake Crisis — GWHB / Game & Watch Sprint Board

Track **changes in this repo only** (`bart_simpson_cupcake_crisis_port`) to produce a single self-contained **`CUPCAKE.bin`** GWHB homebrew (code + all game assets embedded at build time).

**Ship model:** one SD file — `/roms/homebrew/CUPCAKE.bin` — no separate asset folder. Hi-scores (and firmware save slots) use writable paths via the ABI; everything else is baked into the binary.

**Attribution:** bundled art/audio originate from the [itch.io RetroFab release](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis). Ship `docs/ATTRIBUTION.md` with releases and add in-game/README credit (see GW-21).

**Out of scope here:** firmware builds, flashing, GWHB loader patches, or submodules inside `game-and-watch-retro-go-sd`. Use a **GWHB-capable** [retro-go-sd](https://github.com/sylverb/game-and-watch-retro-go-sd) firmware as-is for the launcher + ABI.

**Related:** gameplay parity → [SPRINT_BOARD.md](SPRINT_BOARD.md).

---

## Scope

| In **this repo** | **External** (you set up once, no tasks here) |
|------------------|-----------------------------------------------|
| `platform/gwhb/` ABI host | GWHB-capable retro-go-sd firmware (external) |
| `Makefile.gwhb` → `build-gwhb/CUPCAKE.bin` (assets embedded) | `make flash_sd GNW_TARGET=mario` in that tree |
| Asset bundle tool (`tools/bundle_gwhb_assets.py`) | arm-none-eabi-gcc, gnwmanager (MSYS2 / WSL) |
| `docs/GWHB*.md`, `docs/ATTRIBUTION.md` | Optional: other GWHB homebrew apps as reference |

---

## Do we need to change upstream firmware?

**No, for normal Cupcake shipping** — if we follow the same contract Doom uses:

1. **`CUPCAKE.bin`** starts with magic `GWHB` (`0x42485747`), entry at **+4** with signature `(load_state, start_paused, save_slot)`.
2. App uses **`gw_firmware_abi` only** (no firmware symbol links).
3. **Assets** embedded in `CUPCAKE.bin` at build time (atlas, bezel, audio) — **not** loose SD files. Only hi-scores / save data write back via ABI.

The GWHB loader already handles any `GWHB` `.bin` in Homebrew; menu title comes from the filename stem (`CUPCAKE.bin` → **CUPCAKE**).

**Size note:** bundling the full atlas + audio increases image size; GW-02 must confirm fit (may use compressed `.rodata`, stage-1 copy, or split ITCM/AXISRAM like other GWHB apps — still one user-facing `.bin` file).

**Open a firmware issue only if** we hit a hard limit (RAM overflow needing loader changes, missing ABI field, FrogFS rule blocking our asset names). GW-02 size spike should catch RAM early; Cupcake is much smaller than Doom.

---

## Gap analysis — this repo today vs GWHB target

| Piece | Today (this repo) | Target (this repo) |
|-------|-------------------|---------------------|
| Game core | `src/cupcake_game.c` | Unchanged |
| Device host | Docs only (`-DCUPCAKE_AUDIO_ODROID`); linux emu uses SDL | `platform/gwhb/` + ABI |
| Display | stb + SDL in `platform/retrogo/main.c` | ABI LCD + shared `host_draw.c` |
| Audio | SDL_mixer (emu); odroid path in `host_audio.c` not wired for GWHB | Embedded WAV table in `.bin`; decode/play from rodata |
| Assets | Runtime PNG/JPG from `CUPCAKE_ASSETS` | Build-time bundle → linked into `CUPCAKE.bin` |
| Ship artifact | N/A | Single `build-gwhb/CUPCAKE.bin` (+ `CREDITS`/zip for GitHub) |

---

## Architecture (files we add/change)

```
src/                          ← unchanged
platform/
  host_draw.c                 ← shared (may need small GWHB ifdef)
  host_audio.c                ← odroid path + ABI fopen
  cupcake_input.c             ← unchanged
  retrogo/                    ← unchanged (linux emu regression)
  gwhb/                       ← NEW
    main_gwhb.c
    gwhb_entry.c              ← only if GW-02 says multi-segment copy needed
    rg_abi.h  gw_firmware_abi.h  abi_stubs.c  rg_lcd.c  rg_input.c
    linker.ld
    Makefile.gwhb
tools/
  bundle_gwhb_assets.py     ← NEW (GW-13): PNG/JPG/WAV → generated pack linked into .bin
docs/
  GWHB.md  GWHB_INSTALL.md  GWHB_SMOKE.md  ATTRIBUTION.md
release/                      ← gitignored: CUPCAKE.bin zip + CREDITS (no asset folder)
```

**User SD layout** (single-file install):

```
/roms/homebrew/CUPCAKE.bin    ← everything (code + art + audio)
/roms/homebrew/CUPCAKE.png    ← optional cover for menu (not bundled in .bin)
```

Hi-scores and emulator save slots use firmware writable storage (not inside the read-only bundle).

---

## Sprint roadmap

| Sprint | Theme | Tasks |
|--------|-------|-------|
| GW-1 | Docs & sizing | GW-01 – GW-03 |
| GW-2 | ABI host layer | GW-04 – GW-07 |
| GW-3 | GWHB link & build | GW-08 – GW-11 |
| GW-4 | Bundled assets (in `.bin`) | GW-12 – GW-15 |
| GW-5 | Main loop & callbacks | GW-16 – GW-20 |
| GW-6 | Release artifacts | GW-21 – GW-23 |
| GW-7 | Hardware validation (manual) | GW-24 – GW-27 |

**Status key:** `[ ]` not started · `[~]` in progress · `[x]` done

---

## Sprint GW-1 — Docs & sizing

| Task | Title | Status |
|------|-------|--------|
| GW-01 | `docs/GWHB.md` — port-side GWHB guide | [ ] |
| GW-02 | RAM / code size spike (single-segment vs stage-1) | [ ] |
| GW-03 | Linux-emu regression guard (`Makefile.cupcake`) | [ ] |

---

### GW-01 `docs/GWHB.md` — port-side GWHB guide

    Type: Docs

    Status: [ ]

    Acceptance Criteria: Documents GWHB header contract, ABI-only rule, contrast with linux emu vs Celeste overlay, **external** GWHB-capable firmware requirement (no steps to patch SylverB in this repo). Lists toolchain deps (arm-none-eabi) with pointer to retro-go/GWHB docs, not a submodule in our tree. SD paths and “when to ask for firmware changes” (see board header).

    Work Summary:

    Feedback/Notes:

---

### GW-02 RAM / code size spike (single-segment vs stage-1)

    Type: Spike

    Status: [ ]

    Acceptance Criteria: Rough `.text+.data+.bss` + **embedded asset pack size** (atlas + bezel + 17 WAVs) for core + host. Compare to retro-go `__RAM_EMU` budget and max practical `CUPCAKE.bin` file size on SD. Record decision in `docs/GWHB.md`: single-segment link vs stage-1 copier vs compression. If overflow, compress pack before asking for firmware changes.

    Work Summary:

    Feedback/Notes: Bundled atlas (~4 MB raw RGBA) dominates — expect compression or GWHB stage-1 copier pattern.

---

### GW-03 Linux-emu regression guard (`Makefile.cupcake`)

    Type: QA

    Status: [ ]

    Acceptance Criteria: After GWHB work touching shared code, `make -f platform/retrogo/Makefile.cupcake` + demo run still pass. Note in PR/commits when `src/` or `host_draw.c` changes.

    Work Summary:

    Feedback/Notes:

---

## Sprint GW-2 — ABI host layer

| Task | Title | Status |
|------|-------|--------|
| GW-04 | Vendor `gw_firmware_abi.h` + `rg_abi.h` | [ ] |
| GW-05 | ABI glue stubs (`abi_stubs.c`) | [ ] |
| GW-06 | LCD RGB565 via ABI (`rg_lcd.c`) | [ ] |
| GW-07 | Input via ABI (`rg_input.c`) | [ ] |

---

### GW-04 Vendor `gw_firmware_abi.h` + `rg_abi.h`

    Type: Feature

    Status: [ ]

    Acceptance Criteria: `platform/gwhb/` contains ABI header copied from retro-go GWHB firmware (v1, do not reorder fields). `rg_abi_init()` checks version/size; shows overlay message on mismatch.

    Work Summary:

    Feedback/Notes:

---

### GW-05 ABI glue stubs (`abi_stubs.c`)

    Type: Feature

    Status: [ ]

    Acceptance Criteria: Freestanding link: `malloc`/`free`/`fopen`/etc. route through `GW_FIRMWARE_ABI`. Minimal set for `host_audio`, hi-score, asset load. Pattern from existing GWHB reference apps (reference only — code lives here).

    Work Summary:

    Feedback/Notes:

---

### GW-06 LCD RGB565 via ABI (`rg_lcd.c`)

    Type: Feature

    Status: [ ]

    Acceptance Criteria: Bezel + LCD composite into firmware framebuffer via `lcd_get_active_buffer` / `lcd_swap`. Reuses `host_bezel_blit_rgb565` / `host_draw` math. No SDL.

    Work Summary:

    Feedback/Notes:

---

### GW-07 Input via ABI (`rg_input.c`)

    Type: Feature

    Status: [ ]

    Acceptance Criteria: `odroid_input_read_gamepad` → `cupcake_input_from_odroid` → `cupcake_set_buttons`. Matches [docs/INPUT_MAPPING.md](docs/INPUT_MAPPING.md).

    Work Summary:

    Feedback/Notes:

---

## Sprint GW-3 — GWHB link & build

| Task | Title | Status |
|------|-------|--------|
| GW-08 | `linker.ld` — `GWHB` magic + entry @+4 | [ ] |
| GW-09 | Entry `cupcake_start(load_state, …)` | [ ] |
| GW-10 | `Makefile.gwhb` → `build-gwhb/CUPCAKE.bin` | [ ] |
| GW-11 | `make test-gwhb-header` | [ ] |

---

### GW-08 `linker.ld` — `GWHB` magic + entry @+4

    Type: Feature

    Status: [ ]

    Acceptance Criteria: `LONG(0x42485747)` at image base; entry at +4; RAM map fits GW-02. `-nostdlib -ffreestanding`.

    Work Summary:

    Feedback/Notes:

---

### GW-09 Entry `cupcake_start(load_state, …)`

    Type: Feature

    Status: [ ]

    Acceptance Criteria: `void cupcake_start(uint8_t load_state, uint8_t start_paused, int8_t save_slot)` — ABI init, assets, `cupcake_init()`, main loop. BSS zeroed before use.

    Work Summary:

    Feedback/Notes:

---

### GW-10 `Makefile.gwhb` → `build-gwhb/CUPCAKE.bin`

    Type: Feature

    Status: [ ]

    Acceptance Criteria: `make -f platform/gwhb/Makefile.gwhb` from port root. Links `src/cupcake_*.c`, `host_draw`, `host_audio` (odroid), `cupcake_input`, `platform/gwhb/*`. Flags: `-DCUPCAKE_GWHB -DCUPCAKE_AUDIO_ODROID`; no SDL, stb, or `LINUX_EMU`. `.github/workflows/build.yml` runs this on every push (toolchain-only until host sources exist).

    Work Summary: Makefile.gwhb scaffold + CI workflow added; full link rules activate when `main_gwhb.c` is present.

    Feedback/Notes:

---

### GW-11 `make test-gwhb-header`

    Type: QA

    Status: [ ]

    Acceptance Criteria: Asserts bytes `47 48 57 42` at 0 and valid thumb entry at +4. `scripts/ci/verify_gwhb_header.sh` + `make -f platform/gwhb/Makefile.gwhb test-header`; CI runs this after GWHB build.

    Work Summary:

    Feedback/Notes:

---

## Sprint GW-4 — Bundled assets (in `.bin`)

All game art and audio are **linked into `CUPCAKE.bin` at build time** (Celeste-style embedded data, or a generated `cupcake_assets.o`). CI bundles from local `assets/` during `make gwhb-release` (assets remain gitignored; CI needs HAR extract step or committed generated pack — see GW-22 notes).

| Task | Title | Status |
|------|-------|--------|
| GW-12 | Asset pack format + linker section | [ ] |
| GW-13 | `tools/bundle_gwhb_assets.py` → link into `.bin` | [ ] |
| GW-14 | Play audio from embedded WAV table | [ ] |
| GW-15 | Hi-scores via firmware writable path only | [ ] |

---

### GW-12 Asset pack format + linker section

    Type: Feature

    Status: [ ]

    Acceptance Criteria: Define `cupcake_asset_pack` layout (header + atlas RGBA, bezel RGB565, indexed audio blobs). Generated object or `.inc` linked as `.rodata` in `linker.ld`. Runtime resolves pointers from `&__cupcake_assets_start` (no `fopen` for game art/audio). Document size budget in `docs/GWHB.md`.

    Work Summary:

    Feedback/Notes:

---

### GW-13 `tools/bundle_gwhb_assets.py` → link into `.bin`

    Type: Feature

    Status: [ ]

    Acceptance Criteria: Reads `assets/sprites-color.png`, `assets/screen.jpg`, `assets/audio/*.wav` (from itch extract); emits `platform/gwhb/cupcake_assets.o` or `.h`+`.c`. `Makefile.gwhb` depends on it — `make gwhb` always rebuilds pack before link. Optional LZMA/size assert against GW-02 limit.

    Work Summary:

    Feedback/Notes: PC/linux emu keeps loading loose files; bundle is GWHB-only.

---

### GW-14 Play audio from embedded WAV table

    Type: Feature

    Status: [ ]

    Acceptance Criteria: `host_audio.c` odroid path loads samples from embedded pack (pointer + length per id), not SD paths. `host_audio_pump` each frame @ 22050 Hz. Same 17 clip ids as PC build.

    Work Summary:

    Feedback/Notes:

---

### GW-15 Hi-scores via firmware writable path only

    Type: Feature

    Status: [ ]

    Acceptance Criteria: `cupcake_hiscore_set_path()` uses firmware save directory via ABI (e.g. alongside other homebrew saves) — **not** inside bundled rodata. Persists across sessions on SD.

    Work Summary:

    Feedback/Notes: Only mutable data lives outside the `.bin` bundle.

---

## Sprint GW-5 — Main loop & callbacks

| Task | Title | Status |
|------|-------|--------|
| GW-16 | 30 FPS loop via ABI (`common_emu_frame_loop`) | [ ] |
| GW-17 | Save / load slots (`odroid_system_emu_*`) | [ ] |
| GW-18 | Port `cupcake_cb_wrap` from retrogo main | [ ] |
| GW-19 | Mute / volume (`CUPCAKE_CB_SOUND_TOGGLE`) | [ ] |
| GW-20 | Menu exit (`odroid_system_switch_app`) | [ ] |

---

### GW-16 30 FPS loop via ABI (`common_emu_frame_loop`)

    Type: Feature

    Status: [ ]

    Acceptance Criteria: `APPID_HOMEBREW`, 22050 Hz audio, 30 Hz frame timing. Loop: input → update → draw → swap → audio → `wdog_refresh`.

    Work Summary:

    Feedback/Notes:

---

### GW-17 Save / load slots (`odroid_system_emu_*`)

    Type: Feature

    Status: [ ]

    Acceptance Criteria: Firmware save slots (not `./cupcake.sav`). Honors GWHB entry `load_state` / `save_slot`. Full state blob (TASK-42).

    Work Summary:

    Feedback/Notes:

---

### GW-18 Port `cupcake_cb_wrap` from retrogo main

    Type: Feature

    Status: [ ]

    Acceptance Criteria: Same callback behavior as `platform/retrogo/main.c` for FRAME/SPR/BTN/SFX/mute, backed by GW-06/GW-14/GW-07.

    Work Summary:

    Feedback/Notes:

---

### GW-19 Mute / volume (`CUPCAKE_CB_SOUND_TOGGLE`)

    Type: Feature

    Status: [ ]

    Acceptance Criteria: PAUSE mute toggle; respects firmware volume; stop SFX on mode changes (TASK-41).

    Work Summary:

    Feedback/Notes:

---

### GW-20 Menu exit (`odroid_system_switch_app`)

    Type: Feature

    Status: [ ]

    Acceptance Criteria: PAUSE/SET menu returns to launcher cleanly.

    Work Summary:

    Feedback/Notes:

---

## Sprint GW-6 — Release artifacts

| Task | Title | Status |
|------|-------|--------|
| GW-21 | `docs/GWHB_INSTALL.md` | [ ] |
| GW-22 | `make gwhb-release` zip target | [ ] |
| GW-23 | Optional `release/CUPCAKE.png` cover source | [ ] |

---

### GW-21 `docs/GWHB_INSTALL.md` + attribution

    Type: Docs

    Status: [ ]

    Acceptance Criteria: User steps: GWHB-capable firmware, copy **only** `CUPCAKE.bin` to `/roms/homebrew/`, optional cover PNG, overclock tip. Link [docs/ATTRIBUTION.md](docs/ATTRIBUTION.md) (itch.io RetroFab credit + port author). In-game or pause-screen credit line if feasible (GW-18 follow-up).

    Work Summary:

    Feedback/Notes:

---

### GW-22 `make gwhb-release` zip target

    Type: Release

    Status: [ ]

    Acceptance Criteria: One command: bundle assets → link `CUPCAKE.bin` → zip with `CUPCAKE.bin`, `ATTRIBUTION.md`, `GWHB_INSTALL.md`. No separate asset directory. Output gitignored under `release/`. CI `build-gwhb` job runs bundle step when assets present (document secret/cache strategy for GitHub if assets stay private).

    Work Summary:

    Feedback/Notes: Local dev runs `make extract-audio` + bundle before release; CI may use workflow_dispatch with cached assets later.

---

### GW-23 Optional `release/CUPCAKE.png` cover source

    Type: Polish

    Status: [ ]

    Acceptance Criteria: Include cover PNG in release zip for users to drop in `/roms/homebrew/` (they run retro-go `gencovers` if wanted). No firmware script changes in our repo.

    Work Summary:

    Feedback/Notes:

---

## Sprint GW-7 — Hardware validation (manual)

Manual testing **using your GWHB firmware** — checklist lives in this repo; execution is not a code task.

| Task | Title | Status |
|------|-------|--------|
| GW-24 | `docs/GWHB_SMOKE.md` + smoke sign-off | [ ] |
| GW-25 | Device parity vs [PARITY_CHECKLIST.md](docs/PARITY_CHECKLIST.md) | [ ] |
| GW-26 | Save-slot stress sign-off | [ ] |
| GW-27 | Release sign-off | [ ] |

---

### GW-24 `docs/GWHB_SMOKE.md` + smoke sign-off

    Type: QA

    Status: [ ]

    Acceptance Criteria: Checklist: flash GWHB firmware → copy release zip to SD → Homebrew → CUPCAKE → demo, input, sound, menu return. Sign-off date in Feedback/Notes below.

    Work Summary:

    Feedback/Notes:

---

### GW-25 Device parity vs PARITY_CHECKLIST

    Type: QA

    Status: [ ]

    Acceptance Criteria: [docs/PARITY_CHECKLIST.md](docs/PARITY_CHECKLIST.md) scenarios pass on hardware. New gameplay bugs → [SPRINT_BOARD.md](SPRINT_BOARD.md), not GWHB board.

    Work Summary:

    Feedback/Notes:

---

### GW-26 Save-slot stress sign-off

    Type: QA

    Status: [ ]

    Acceptance Criteria: Slots 0–3 save/load + power cycle; hi-scores intact.

    Work Summary:

    Feedback/Notes:

---

### GW-27 Release sign-off

    Type: Release

    Status: [ ]

    Acceptance Criteria: GW-01–GW-23 complete; GW-24–26 signed off. Record tested GWHB firmware commit/branch in Feedback/Notes.

    Work Summary:

    Feedback/Notes:

---

## Dependency graph

```
GW-01 ── GW-02 ── GW-08 ── GW-10 ── GW-11
          │                    ↓
GW-03     └── GW-04..07 ── GW-09 ── GW-12 ── GW-13 ── GW-16..20 ── GW-22
                              GW-14,15 ↗              ↘ GW-21,23 → GW-27
GW-24..26 (manual, after GW-22)
```

---

## Already in this repo (reuse)

- `src/cupcake_game.c` + tests ([SPRINT_BOARD.md](SPRINT_BOARD.md))
- `platform/host_draw.c`, `cupcake_input.c`, odroid audio design in `host_audio.c`
- `platform/retrogo/` linux emu for regression (GW-03)

---

## Quick status

| Sprint | Done | Total |
|--------|------|-------|
| GW-1 | 0 | 3 |
| GW-2 | 0 | 4 |
| GW-3 | 0 | 4 |
| GW-4 | 0 | 4 |
| GW-5 | 0 | 5 |
| GW-6 | 0 | 3 |
| GW-7 | 0 | 4 |
| **All** | **0** | **27** |
