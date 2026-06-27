# Game & Watch device overlay host

Implements `app_main_cupcake()` for the Celeste-model RAM overlay on retro-go G&W firmware.

## Files

| File | Role |
|------|------|
| `main_cupcake.c` | Firmware entry — emu loop, LCD blit, input, audio, save states |
| `gnw_assets.c` | Load bezel + sprite atlas from SD (interim until OV-3 embedded pack) |
| `cupcake_overlay.mk` | Source list + flags included by firmware when `CUPCAKE_PORT` is set |
| `Makefile.overlay` | Convenience wrapper → `make cupcake-overlay-bin` in firmware tree |

## Build `cupcake.bin`

Requires `arm-none-eabi-gcc` and firmware submodule `retro-go-stm32`:

```bash
cd /path/to/game-and-watch-retro-go-sd-cupcake
git submodule update --init retro-go-stm32

cd /path/to/bart_simpson_cupcake_crisis_port
make -f platform/gnw/Makefile.overlay
```

Output lands in the firmware SD homebrew folder (see `HOMEBREWS_FOLDER` in firmware `Makefile.common`). Copy to device: `/roms/homebrew/cupcake.bin`.

## SD assets (interim)

Until embedded assets land (OV-3), copy from port `assets/`:

```
/retro-go/cupcake/screen.jpg
/retro-go/cupcake/sprites-color.png
/retro-go/cupcake/audio/*.wav
```

Hi-scores: `/retro-go/saves/cupcake_hiscores.dat`.

See [docs/OVERLAY.md](../../docs/OVERLAY.md).
