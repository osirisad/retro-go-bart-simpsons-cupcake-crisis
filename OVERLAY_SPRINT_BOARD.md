# Cupcake Crisis — Overlay / Game & Watch Sprint Board

**Active ship path:** Celeste-style **named overlay** on stable `game-and-watch-retro-go-sd-cupcake` firmware.

**GWHB is deferred** — see [GWHB_SPRINT_BOARD.md](GWHB_SPRINT_BOARD.md) (on hold). Do not add GWHB loader code to firmware for this round.

## How it works (same as Celeste)

1. Cupcake links into firmware RAM overlay section `.overlay_cupcake` at `__RAM_EMU_START__` (`0x2404B000`).
2. Firmware build extracts `cupcake.bin` → copy to `/roms/homebrew/cupcake.bin`.
3. Launcher loads the bin into RAM, matches filename stem **`cupcake`**, zeros overlay BSS, calls **`app_main_cupcake()`** (fixed RAM entry — same pattern as `app_main_celeste`).
4. Assets embedded at build time (`cupcake_data.h`), not loose SD files.

**Port repo stays the source of truth:** firmware `Makefile.common` pulls Cupcake sources via `CUPCAKE_PORT` (like `platform/retrogo/Makefile.cupcake` for linux emu). You do not duplicate game logic inside the firmware tree.

**Attribution:** [docs/ATTRIBUTION.md](docs/ATTRIBUTION.md).

**Related:** gameplay parity → [SPRINT_BOARD.md](SPRINT_BOARD.md).

---

## Scope

| In **port repo** | In **firmware fork** (`game-and-watch-retro-go-sd-cupcake`) |
|------------------|---------------------------------------------------------------|
| `src/`, `platform/host_*.c`, `platform/gnw/main_cupcake.c` | `.overlay_cupcake` + `.overlay_cupcake_bss` in linker scripts |
| `tools/bundle_overlay_assets.py` → `cupcake_data.h` | `Makefile.common` — `CUPCAKE_C_SOURCES` via `CUPCAKE_PORT` |
| Linux emu regression (`Makefile.cupcake`) | `rg_emulators.c` — `strcmp(name,"cupcake")` + `app_main_cupcake` |
| | `objcopy` → `cupcake.bin` in `HOMEBREWS_FOLDER` |

**RAM budget:** `__RAM_EMU_LENGTH__` ≈ 724 KiB (SD linker). OV-02 must confirm atlas + audio fit.

---

## Gap analysis — today vs overlay target

| Piece | Today | Target |
|-------|-------|--------|
| Game core | `src/cupcake_game.c` | Unchanged |
| Device host | `platform/retrogo/main.c` (linux emu only) | `platform/gnw/main_cupcake.c` |
| Display | SDL / stb in emu | `gw_lcd` + shared `host_draw.c` |
| Audio | SDL_mixer (emu); odroid path stubbed | `odroid_audio` + embedded WAV table |
| Assets | Runtime PNG/JPG from `CUPCAKE_ASSETS` | Build-time bundle → `cupcake_data.h` |
| Firmware | Not integrated | Overlay section + `"cupcake"` dispatch |
| Ship artifact | `retro-go-cupcake.elf` (linux emu) | `cupcake.bin` + flashed firmware |

---

## Sprint OV-1 — Spike & docs

| Task | Title | Status |
|------|-------|--------|
| OV-01 | `docs/OVERLAY.md` — Celeste-model guide | [x] |
| OV-02 | RAM / overlay size budget vs `__RAM_EMU` | [x] |
| OV-03 | Linux emu regression gate | [x] |

### OV-01 `docs/OVERLAY.md`

    Acceptance Criteria: Documents Celeste flow (link section → extract bin → SD → name dispatch → `app_main_*`), `CUPCAKE_PORT` wiring, contrast with deferred GWHB path.

### OV-02 RAM / overlay size budget

    Acceptance Criteria: Measure `.text+.data+.bss` + embedded asset pack vs Celeste overlay usage and `__RAM_EMU` limit. Record compression decision if needed.

### OV-03 Linux emu regression gate

    Acceptance Criteria: `make test-overlay-regression` runs PC smoke + demo-replay + host-bezel tests; `scripts/overlay_regression.sh` builds `Makefile.cupcake` when `RETROGO_FW` (or sibling firmware tree) is present. PC-only: `make test-overlay-regression-pc`.

---

## Sprint OV-2 — Port device host

| Task | Title | Status |
|------|-------|--------|
| OV-04 | `platform/gnw/main_cupcake.c` skeleton | [x] |
| OV-05 | Odroid LCD + input wiring | [x] |
| OV-06 | Odroid audio (SD WAV interim; embedded in OV-3) | [x] |
| OV-07 | `host_draw.c` / `host_audio.c` GNW ifdef pass | [x] |

### OV-04 `main_cupcake.c`

    Full overlay host: `app_main_cupcake`, `common_emu_frame_loop`, save/load, blit to `gw_lcd`. Interim SD assets via `gnw_assets.c` until OV-3 embedded pack.

### OV-05 LCD + input

    `host_lcd_blit_rgb565` → `lcd_get_active_buffer` / `lcd_swap`. Input via `cupcake_input_from_odroid`.

### OV-06 Odroid audio

    `host_audio_init` + `host_audio_pump` with `CUPCAKE_AUDIO_ODROID`. WAVs from `/retro-go/cupcake/audio/` on SD until OV-08/09 embed catalog.

### OV-07 Host ifdef pass

    `host_draw.c` stays platform-neutral. `host_audio.c` default base `/retro-go/cupcake` when `CUPCAKE_GNW`. Build via `platform/gnw/Makefile.overlay` → `make cupcake-overlay-bin` in firmware with `CUPCAKE_PORT` set.

## Sprint OV-3 — Embedded assets

| Task | Title | Status |
|------|-------|--------|
| OV-08 | `tools/bundle_overlay_assets.py` | [ ] |
| OV-09 | `cupcake_data.h` generated pack | [ ] |
| OV-10 | Hi-scores on firmware writable path | [ ] |

---

## Sprint OV-4 — Firmware integration (external fork)

| Task | Title | Status |
|------|-------|--------|
| OV-11 | Linker `.overlay_cupcake` sections | [x] |
| OV-12 | `Makefile.common` — `CUPCAKE_C_SOURCES` + extract | [x] |
| OV-13 | `rg_emulators.c` — `strcmp(name,"cupcake")` | [x] |
| OV-14 | Smoke-test stub on hardware | [ ] |
| OV-15 | `docs/OVERLAY_INSTALL.md` | [ ] |

### OV-11 Linker sections

    Acceptance Criteria: Mirror `.overlay_celeste` in `STM32H7B0VBTx_*.ld` — load addr, BSS, overflow ASSERT. Export symbols in `gw_linker.h`.

### OV-12 Makefile integration

    Acceptance Criteria: `CUPCAKE_PORT` points at port repo; `-DCUPCAKE_GNW -DCUPCAKE_AUDIO_ODROID`. `objcopy --only-section=.overlay_cupcake` → `HOMEBREWS_FOLDER/cupcake.bin`.

### OV-13 Homebrew dispatch

    Acceptance Criteria: Under `system_name == "Homebrew"`, after SD load: `strcmp(newfile->name,"cupcake")` → zero BSS, cache flush, `app_main_cupcake(...)`.

### OV-14 Hardware smoke

    Acceptance Criteria: Stub `app_main_cupcake` runs on device; returns to menu.

### OV-15 Install doc

    Acceptance Criteria: Flash firmware once; copy `cupcake.bin`; link [ATTRIBUTION.md](docs/ATTRIBUTION.md).

---

## Sprint OV-5 — Polish & ship

| Task | Title | Status |
|------|-------|--------|
| OV-16 | Save states via firmware slots | [ ] |
| OV-17 | Hardware parity sign-off | [ ] |

---

## Dependency graph

```
OV-01 ── OV-02 ── OV-11 ── OV-12 ── OV-13 ── OV-14
          │                    ↓
OV-03     └── OV-04..07 ── OV-08 ── OV-09 ── OV-16..17
                              └── OV-10
                                    OV-15
```

---

## When to revisit GWHB

Resume [GWHB_SPRINT_BOARD.md](GWHB_SPRINT_BOARD.md) if you later want bin-only updates without reflashing firmware, or a self-contained ABI-only image built entirely outside the firmware ELF.
