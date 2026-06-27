# Vendored API headers for standalone `cupcake.bin` builds.

These are **declarations only** — enough to compile the overlay in this repo without
checking out `game-and-watch-retro-go-sd-cupcake`.

Firmware **function addresses** come from `../firmware_imports.ld` (or `FIRMWARE_ELF=`
when regenerating that file).

If the firmware API changes, update these stubs and regenerate `firmware_imports.ld`.
