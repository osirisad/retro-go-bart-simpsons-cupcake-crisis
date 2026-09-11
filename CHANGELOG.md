# Changelog

## [v1.3.1] - 2026-09-11

### Changed

- Ship as a Retro-Go SD GWHB homebrew: single `Cupcake.bin` in `/homebrews/`
  (graphics + ADPCM embedded; no `cupcake_assets.dat`).
- Pause + d-pad shows the firmware volume/brightness HUD.

### Install

- Copy `Cupcake.bin` to `/homebrews/Cupcake.bin` (or unzip the release
  archive onto the SD card root).
- Requires Retro-Go SD firmware that loads GWHB homebrews (ABI v2+).

## [v0.0.1]

### Added

- Port Cupcake Crisis as a GWHB homebrew on the Retro-Go SD template
  (ABI v2 / `gw_core_bridge`, embedded RGB565 art + ADPCM audio).

### Changed

- Package layout: single `/homebrews/Cupcake.bin` (audio embedded; no
  `cupcake_assets.dat` sidecar).
- Draw directly into the LCD buffer and play ADPCM from the embedded
  archive so the audio blob fits in RAM_EMU.

### Install

- Unzip the release archive onto the SD card root (`homebrews/Cupcake.bin`).
- Requires firmware whose ABI matches `SDK_VERSION` in this repository.
