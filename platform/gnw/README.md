# Game & Watch device overlay host

Implements `app_main_cupcake()` for the Celeste-model RAM overlay on retro-go G&W firmware.

## Build `cupcake.bin` (standalone — no firmware source)

```bash
make cupcake-bin
# or: make -f platform/gnw/Makefile.gnw
```

Output: **`release/cupcake.bin`** → copy to **`/roms/homebrew/cupcake.bin`** on SD.

Requires **`arm-none-eabi-gcc`** only. Runtime firmware calls go through **`gw_firmware_abi`** (`abi_stubs.c`).

Plain-language guide: [docs/RELEASE_CUPCAKE_BIN.md](../../docs/RELEASE_CUPCAKE_BIN.md).

## SD assets (interim until RG-4)

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
| `abi_stubs.c` | libc + retro-go shims via `gw_firmware_abi` |
| `rg_abi.h` | ABI version check, `common_emu_state` accessor |
| `gw_firmware_abi.h` | Vendored ABI struct (sync on version bump) |
| `Makefile.gnw` | Standalone ARM build |
| `overlay.ld` | RAM overlay linker layout |
| `sdk/` | Vendored compile-time API headers |
