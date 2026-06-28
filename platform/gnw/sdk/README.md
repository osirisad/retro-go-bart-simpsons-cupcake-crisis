# Vendored API headers for standalone `cupcake.bin` builds

These are **declarations only** — enough to compile the overlay in this repo without
checking out `game-and-watch-retro-go-sd-cupcake`.

Firmware **function calls** at runtime go through **`gw_firmware_abi`** (see
`../abi_stubs.c` and `../gw_firmware_abi.h`). No `firmware.elf` or symbol-import
linker script is required at build time.

When firmware bumps **`GW_FIRMWARE_ABI_VERSION`**, sync `gw_firmware_abi.h` from
upstream and extend `abi_stubs.c` if new fields are needed.
