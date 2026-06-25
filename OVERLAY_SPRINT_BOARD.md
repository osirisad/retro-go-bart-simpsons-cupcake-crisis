# Cupcake Crisis — Overlay / Game & Watch Sprint Board

**Active ship path** — build **`cupcake.bin` entirely in this port repo**, copy to `/roms/homebrew/` on SD. The firmware fork needs a **small one-time patch** (loader dispatch only); it does **not** compile Cupcake sources.

**Why not compile into firmware (classic Celeste):** SylverB’s Celeste flow links game objects inside `game-and-watch-retro-go-sd` and extracts `celeste.bin` from the firmware ELF. That keeps the game tied to the firmware tree. You want the opposite: all Simpson port code stays here; firmware only knows how to load and run the bin.

**Loader options** (pick one in OV-02):

| Model | Port repo output | Firmware change | Updates |
|-------|------------------|-----------------|---------|
| **GWHB generic** (recommended) | `cupcake.bin` with `GWHB` magic + entry @+4 | ~7 lines in `rg_emulators.c` + `gw_firmware_abi` (already on your fork) | Copy new bin only |
| **Named overlay** | Raw overlay bin @ `__RAM_EMU_START__` | `strcmp(name,"cupcake")` + BSS zero + jump to fixed entry offset | Copy new bin only |

Do **not** flash the experimental GWHB firmware tree wholesale — cherry-pick only the generic homebrew dispatch into your stable `game-and-watch-retro-go-sd-cupcake` fork.

**Why overlay board vs GWHB board:** Same end-user experience (one bin on SD). GWHB is the cleaner contract (`gw_firmware_abi`, self-describing header). The [GWHB_SPRINT_BOARD.md](GWHB_SPRINT_BOARD.md) scaffold still applies for port-side build; this board tracks firmware integration on your stable fork.

**Attribution:** [docs/ATTRIBUTION.md](docs/ATTRIBUTION.md) — itch.io RetroFab credit.

**Related:** gameplay parity → [SPRINT_BOARD.md](SPRINT_BOARD.md).

---

## Scope

| In **port repo** (all game code) | In **firmware fork** (one-time, minimal) |
|----------------------------------|------------------------------------------|
| `platform/gnw/` — device host + `linker.ld` @ `__RAM_EMU_START__` | `rg_emulators.c` — load bin → jump (GWHB magic or `"cupcake"` branch) |
| `tools/bundle_overlay_assets.py` → embedded atlas/audio | `gw_firmware_abi` — already present; no game sources added |
| `make` → `build-gnw/cupcake.bin` | Optional: `scripts/size.sh` line for cupcake budget |
| Linux emu regression (`Makefile.cupcake`) | **No** `CUPCAKE_C_SOURCES` in `Makefile.common` |

**Load address:** `__RAM_EMU_START__` = `0x2404B000` (300K UC framebuffer + base `0x24000000`). Budget ≈724K on SD-card linker layout — plenty for Cupcake with embedded assets.

---

## Gap analysis — today vs overlay target

| Piece | Today | Target |
|-------|-------|--------|
| Game core | `src/cupcake_game.c` | Unchanged |
| Device host | `platform/retrogo/main.c` (linux emu only) | `platform/gnw/main_cupcake.c` + odroid LCD/audio |
| Display | SDL / stb in emu | `gw_lcd` + shared `host_draw.c` |
| Audio | SDL_mixer (emu); odroid path stubbed | `odroid_audio` + embedded WAV table |
| Assets | Runtime PNG/JPG from `CUPCAKE_ASSETS` | Build-time bundle → `cupcake_data.h` |
| Firmware | Not integrated | Overlay section + homebrew dispatch |
| Ship artifact | `retro-go-cupcake.elf` (linux emu) | `cupcake.bin` overlay extract + flashed firmware |

---

## Sprint OV-1 — Spike & docs

| Task | Title | Status |
|------|-------|--------|
| OV-01 | `docs/OVERLAY.md` — Celeste-model guide | [ ] |
| OV-02 | RAM / overlay size budget vs `__RAM_EMU` | [ ] |
| OV-03 | Linux emu regression gate | [ ] |

### OV-01 `docs/OVERLAY.md`

    Acceptance Criteria: Documents overlay flow (link section → extract bin → SD homebrew → dispatch), contrast with GWHB (deferred), Celeste reference files in firmware tree, `app_main_cupcake(load_state, start_paused, save_slot)` contract.

### OV-02 RAM / overlay size budget

    Acceptance Criteria: Measure `.text+.data+.bss` + embedded asset pack. Compare to Celeste overlay usage and `__RAM_EMU` limit in linker. Record single-load vs compression decision in `docs/OVERLAY.md`.

### OV-03 Linux emu regression gate

    Acceptance Criteria: After shared-code changes, `make -f platform/retrogo/Makefile.cupcake` + demo run still pass.

---

## Sprint OV-2 — Port device host

| Task | Title | Status |
|------|-------|--------|
| OV-04 | `platform/gnw/main_cupcake.c` skeleton | [ ] |
| OV-05 | Odroid LCD + input wiring | [ ] |
| OV-06 | Odroid audio from embedded WAVs | [ ] |
| OV-07 | `host_draw.c` / `host_audio.c` GNW ifdef pass | [ ] |

### OV-04 `main_cupcake.c` skeleton

    Acceptance Criteria: `app_main_cupcake()` init/teardown, main loop calling `cupcake_game_tick` / `cupcake_draw`, frame pacing ~30 FPS, clean return to launcher.

### OV-05 Odroid LCD + input

    Acceptance Criteria: Map GW buttons via `gw_buttons` / `cupcake_input.c`. Blit through `gw_lcd` like Celeste `blit()`.

### OV-06 Odroid audio

    Acceptance Criteria: `-DCUPCAKE_AUDIO_ODROID`; play embedded WAV catalog without SDL. No runtime `fopen` for game audio.

### OV-07 Shared host ifdef pass

    Acceptance Criteria: `host_draw.c` and `host_audio.c` compile for GNW without SDL/stb file I/O when assets come from `cupcake_data.h`.

---

## Sprint OV-3 — Embedded assets

| Task | Title | Status |
|------|-------|--------|
| OV-08 | `tools/bundle_overlay_assets.py` | [ ] |
| OV-09 | `cupcake_data.h` generated pack | [ ] |
| OV-10 | Hi-scores on firmware writable path | [ ] |

### OV-08 Asset bundle tool

    Acceptance Criteria: PNG/JPG/WAV → C header or `.inc` with atlas RGBA, bezel RGB565, indexed audio blobs. Invoked from firmware `make` or port `make bundle-overlay`.

### OV-09 Generated pack linked in overlay

    Acceptance Criteria: Runtime resolves pointers from embedded tables; document size in `docs/OVERLAY.md`.

### OV-10 Hi-scores persistence

    Acceptance Criteria: Use firmware save path (not `./cupcake.hiscore` on SD root). Same semantics as PC port.

---

## Sprint OV-4 — Firmware integration (external fork, minimal)

| Task | Title | Status |
|------|-------|--------|
| OV-11 | Cherry-pick GWHB dispatch into `rg_emulators.c` | [ ] |
| OV-12 | Smoke-test loader with stub bin | [ ] |
| OV-13 | Document firmware commit / flash once | [ ] |

### OV-11 GWHB dispatch (recommended)

    Acceptance Criteria: After `odroid_overlay_cache_file_in_ram`, if `*(uint32_t*)__RAM_EMU_START__ == 0x42485747`, cache flush + jump to `__RAM_EMU_START__+4` (thumb). No Cupcake symbols linked into firmware. Can copy verbatim from experimental GWHB branch — do not merge whole firmware.

### OV-12 Stub bin smoke test

    Acceptance Criteria: Tiny `cupcake.bin` built in port repo runs on hardware: shows test pattern, returns to menu.

### OV-13 One-time flash doc

    Acceptance Criteria: `docs/OVERLAY_INSTALL.md` — flash patched firmware once; all game updates are SD bin copy only.

---

## Sprint OV-5 — Polish & ship

| Task | Title | Status |
|------|-------|--------|
| OV-16 | Save states via firmware slots | [ ] |
| OV-17 | `docs/OVERLAY_INSTALL.md` | [ ] |
| OV-18 | Hardware smoke + parity sign-off | [ ] |

### OV-16 Save states

    Acceptance Criteria: Honor `load_state` / `save_slot` entry args; full state blob (TASK-42).

### OV-17 Install doc

    Acceptance Criteria: Flash firmware, copy `cupcake.bin`, optional cover PNG, link [ATTRIBUTION.md](docs/ATTRIBUTION.md).

### OV-18 Hardware QA

    Acceptance Criteria: [PARITY_CHECKLIST.md](docs/PARITY_CHECKLIST.md) on device; record tested firmware commit in Feedback/Notes.

---

## Dependency graph

```
OV-01 ── OV-02 ── OV-11 ── OV-12 ── OV-13
          │                    ↓
OV-03     └── OV-04..07 ── OV-08 ── OV-09 ── OV-16..18
                              └── OV-10
```

---

## When to revisit GWHB

Resume [GWHB_SPRINT_BOARD.md](GWHB_SPRINT_BOARD.md) when:

- GWHB loader firmware is stable on your hardware (no regressions vs stock retro-go-sd)
- You want SD-drop updates without reflashing firmware
- Overlay RAM budget is too tight even with compression

Until then, overlay is the pragmatic path.
