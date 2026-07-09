# Game & Watch GWHB device build

`make cupcake-bin` produces **`release/cupcake.bin`** — a generic Universal
Homebrew Header binary for the `gwhb-generic-homebrew-loader` firmware branch.
Any filename under `/roms/homebrew/` works; dispatch is by the 512-byte header,
not by name.

## SD install

| File | SD path |
|------|---------|
| `release/cupcake.bin` | `/roms/homebrew/cupcake.bin` |
| `release/cupcake_assets.dat` | `/roms/homebrew/cupcake_assets.dat` |

Requires **`arm-none-eabi-gcc`**. Runtime firmware calls go through
**`gw_firmware_abi`** (`abi_stubs.c`).

## Files in this directory

| File | Role |
|------|------|
| `main_cupcake.c` | Game loop, LCD, input, audio |
| `gnw_assets.c` | Load bezel + atlas (embedded or SD fallback) |
| `abi_stubs.c` | libc + retro-go shims via `gw_firmware_abi` |
| `rg_abi.h` | ABI version check, `common_emu_state` accessor |
| `gw_firmware_abi.h` | Vendored ABI struct (sync on version bump) |
| `Makefile.gnw` | Standalone ARM build |
| `sdk/` | Vendored compile-time API headers |

GWHB header, entry, and linker script live in [`../gwhb/`](../gwhb/).
