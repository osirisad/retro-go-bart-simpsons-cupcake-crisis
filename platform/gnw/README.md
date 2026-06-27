# Game & Watch device overlay host

Implements `app_main_cupcake()` for the Celeste-model RAM overlay on retro-go G&W firmware.

## Build `cupcake.bin` (standalone — no firmware source)

```bash
make cupcake-bin
# or: make -f platform/gnw/Makefile.gnw
```

Output: **`release/cupcake.bin`** → copy to **`/roms/homebrew/cupcake.bin`** on SD.

Requires `arm-none-eabi-gcc` and `platform/gnw/firmware_imports.ld` (symbol map for your flashed firmware).

Plain-language guide: [docs/RELEASE_CUPCAKE_BIN.md](../../docs/RELEASE_CUPCAKE_BIN.md).

## SD assets (interim until OV-3)

```
/retro-go/cupcake/screen.jpg
/retro-go/cupcake/sprites-color.png
/retro-go/cupcake/audio/*.wav
```

## Files

| File | Role |
|------|------|
| `main_cupcake.c` | Firmware entry — game loop, LCD, input, audio |
| `gnw_assets.c` | Load bezel + atlas from SD |
| `Makefile.gnw` | Standalone ARM build |
| `overlay.ld` | RAM overlay linker layout |
| `firmware_imports.ld` | Firmware symbol addresses (regenerate when firmware changes) |
| `sdk/` | Vendored compile-time API headers |
