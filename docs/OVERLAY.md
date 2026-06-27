# Game & Watch overlay port (Celeste model)

Cupcake Crisis ships on **retro-go Game & Watch** as a firmware **RAM overlay** — the same pattern as Celeste Classic in [game-and-watch-retro-go-sd-cupcake](https://github.com/sylverb/game-and-watch-retro-go-sd).

This repo stays the **source of truth** for game logic and the device host. The firmware fork owns the linker slot, launcher dispatch, and (for now) a smoke-test stub.

**Related:** [OVERLAY_RAM_BUDGET.md](OVERLAY_RAM_BUDGET.md) · [BUILD.md](BUILD.md) · [OVERLAY_SPRINT_BOARD.md](../OVERLAY_SPRINT_BOARD.md) · firmware [OVERLAY_CUPCAKE.md](../../game-and-watch-retro-go-sd-cupcake/docs/OVERLAY_CUPCAKE.md)

---

## Three build targets (one game core)

| Target | Host | Build | Assets |
|--------|------|-------|--------|
| **PC / Linux dev** | `platform/sdl/main.c` | `make` → `build-pc/cupcake-sdl` | Loose files under `assets/` (`CUPCAKE_ASSETS`) |
| **Linux emu (G&W regression)** | `platform/retrogo/main.c` | `make -f platform/retrogo/Makefile.cupcake` | Loose files (default `/home/odroid/cupcake/`) |
| **G&W hardware** | `platform/gnw/main_cupcake.c` | `cupcake.bin` on SD (see below) | Embedded `cupcake_data.h` at build time |

Only the **platform host** and **asset loading path** differ. Game rules live in `src/cupcake_game.c` and are shared.

### Compile flags (by target)

| Flag | PC / linux emu | Device overlay |
|------|----------------|----------------|
| `LINUX_EMU` | linux emu only | — |
| `CUPCAKE_GNW` | — | device overlay build |
| `CUPCAKE_AUDIO_ODROID` | optional in emu | device audio path in `host_audio.c` |
| `CUPCAKE_EMBEDDED_ASSETS` | — | planned (OV-08); PC/emu keep filesystem assets |

After embedded-asset work lands, **PC and linux emu keep loading PNG/JPG/WAV from disk** unless you opt in explicitly. Shared `src/` changes must pass `make test-overlay-regression` (see [Regression gate](#regression-gate-ov-03)).

---

## Celeste overlay flow (what firmware does)

All homebrew overlays share one RAM window at `__RAM_EMU_START__` (`0x2404B000` on SD layout). Only one overlay runs at a time.

```
Firmware link
  └─► .overlay_cupcake  (code + .data + .rodata)  @ __RAM_EMU_START__
  └─► .overlay_cupcake_bss                         immediately after load image

SD image build (objcopy)
  └─► /roms/homebrew/cupcake.bin   (raw load image — .text/.data/.rodata only)

User: Homebrew → cupcake
  └─► odroid_overlay_cache_file_in_ram(path, __RAM_EMU_START__)
  └─► memset(.overlay_cupcake_bss)
  └─► SCB_CleanDCache_by_Addr(...)
  └─► app_main_cupcake(load_state, start_paused, save_slot)
```

Entry signature (fixed ABI — do not change without reflashing firmware):

```c
void app_main_cupcake(uint8_t load_state, uint8_t start_paused, int8_t save_slot);
```

Declared in firmware `Core/Inc/porting/cupcake/main_cupcake.h`; implemented in this repo as `platform/gnw/main_cupcake.c`.

### Filename dispatch

| SD path | Menu stem | Dispatch |
|---------|-----------|----------|
| `/roms/homebrew/cupcake.bin` | **cupcake** | `strcmp(name,"cupcake")` in `rg_emulators.c` |

Same idea as `celeste.bin` → `"celeste"` → `app_main_celeste()`.

---

## What lives where

| Piece | Port repo (`bart_simpson_cupcake_crisis_port`) | Firmware fork |
|-------|-----------------------------------------------|---------------|
| Game core | `src/cupcake_*.c` | — |
| Shared draw/audio/input | `platform/host_draw.c`, `host_audio.c`, `cupcake_input.c` | — |
| Device entry | `platform/gnw/main_cupcake.c` | stub until full `cupcake.bin` replaces SD file |
| Linux emu host | `platform/retrogo/main.c`, `Makefile.cupcake` | — |
| Asset bundle tool | `tools/bundle_overlay_assets.py` (OV-08) | — |
| Linker sections | — | `.overlay_cupcake` / `_bss` in `STM32H7B0VBTx_*.ld` |
| Launcher | — | `rg_emulators.c` Homebrew branch |
| Stub firmware build | — | `Core/Src/porting/cupcake/main_cupcake.c` only |
| Extract to SD | — | `objcopy --only-section=.overlay_cupcake` → `cupcake.bin` |

### Two ways to produce `cupcake.bin`

1. **Firmware stub (today)** — build firmware alone; ships a tiny smoke-test `cupcake.bin`. No `CUPCAKE_PORT` required. See firmware [OVERLAY_CUPCAKE.md](../../game-and-watch-retro-go-sd-cupcake/docs/OVERLAY_CUPCAKE.md).

2. **Full game (target)** — build from this port repo (`platform/gnw/Makefile.overlay`, OV-04b), copy output to `/roms/homebrew/cupcake.bin`. Firmware **does not** need a rebuild unless the overlay ABI or linker layout changes.

Optional dev shortcut: point firmware `CUPCAKE_C_SOURCES` at this repo (like `Makefile.cupcake` for linux emu) and `objcopy` from the firmware ELF. That is convenient for all-in-one firmware dev, not required for end-user SD drops.

---

## `CUPCAKE_PORT` wiring

`CUPCAKE_PORT` is the absolute path to **this repo**. It is already used by the linux emu fragment:

```bash
export CUPCAKE_PORT=/path/to/bart_simpson_cupcake_crisis_port
cd /path/to/game-and-watch-retro-go-sd-cupcake/linux
make -f $CUPCAKE_PORT/platform/retrogo/Makefile.cupcake
```

The fragment pulls shared sources from `$CUPCAKE_PORT/src/` and `$CUPCAKE_PORT/platform/` — same list the device overlay will use, plus `LINUX_EMU`, SDL, and stb for host I/O.

For device overlay (planned `platform/gnw/Makefile.overlay`):

- Toolchain: `arm-none-eabi-gcc`
- Flags: `-DCUPCAKE_GNW -DCUPCAKE_AUDIO_ODROID -DCUPCAKE_EMBEDDED_ASSETS`
- Link: firmware `.overlay_cupcake` linker fragment (same VMA/LMA as Celeste slot)
- Output: `build-gnw/cupcake.bin` via `objcopy --only-section=.overlay_cupcake`

---

## Overlay vs GWHB (deferred)

| | **Overlay (active)** | **GWHB (on hold)** |
|---|---------------------|-------------------|
| Ship file | `cupcake.bin` (lowercase) | `CUPCAKE.bin` + `GWHB` magic |
| Firmware | Named slot + linker section in your fork | Any GWHB-capable retro-go-sd (no Cupcake-specific patch) |
| Entry | `app_main_cupcake(...)` at fixed RAM layout | Thumb fn ptr at offset +4 in standalone bin |
| API | Firmware symbols (`odroid_*`, `gw_lcd`, …) | `gw_firmware_abi` only |
| Port host | `platform/gnw/` | `platform/gwhb/` |
| Update game | Replace SD `cupcake.bin` | Replace SD `CUPCAKE.bin` |
| Sprint board | [OVERLAY_SPRINT_BOARD.md](../OVERLAY_SPRINT_BOARD.md) | [GWHB_SPRINT_BOARD.md](../GWHB_SPRINT_BOARD.md) |

Do **not** mix GWHB loader changes into the overlay firmware round. Revisit GWHB if you want ABI-only bins built entirely outside the firmware ELF.

---

## Developer workflow

### PC (primary)

```bash
make
make test-overlay-regression   # after touching src/ or platform/host_*.c
make run
```

### Linux emu (G&W input/display regression)

Requires retro-go `linux/` tree + SDL2. From firmware `linux/`:

```bash
export CUPCAKE_PORT=/path/to/bart_simpson_cupcake_crisis_port
make -f $CUPCAKE_PORT/platform/retrogo/Makefile.cupcake
# optional manual run: ./build-cupcake/retro-go-cupcake.elf
```

Copy `assets/sprites-color.png`, `assets/screen.jpg`, `assets/audio/*.wav` to `/home/odroid/cupcake/` (or set `CUPCAKE_ASSETS`).

### Device smoke (firmware stub)

1. Flash firmware from `game-and-watch-retro-go-sd-cupcake`.
2. Confirm `/roms/homebrew/cupcake.bin` on SD.
3. Homebrew → **cupcake** — stub runs briefly, returns to menu.

Replace `cupcake.bin` with the port-repo build when OV-4+ land.

---

## Regression gate (OV-03)

After any change under `src/` or `platform/host_*.c` / `cupcake_input.c`:

```bash
make test-overlay-regression
```

Runs headless unit tests (smoke, demo replay, bezel math). On Linux with firmware checked out, also builds `Makefile.cupcake`:

```bash
./scripts/overlay_regression.sh
```

See [BUILD.md](BUILD.md) for toolchain setup.

---

## RAM budget

The overlay must fit in `__RAM_EMU_LENGTH__` (~724 KiB on SD layout). Sprite masks and embedded art dominate size — see [OVERLAY_RAM_BUDGET.md](OVERLAY_RAM_BUDGET.md).

Measure on firmware ELF after link:

```bash
./scripts/size.sh build/your-firmware.elf   # line: ram_emu_cupcake
python tools/overlay_size_estimate.py       # estimate from assets/ when present
```

---

## Attribution

Ship [ATTRIBUTION.md](ATTRIBUTION.md) with `cupcake.bin` releases. Art and audio originate from the [itch.io RetroFab release](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis).
