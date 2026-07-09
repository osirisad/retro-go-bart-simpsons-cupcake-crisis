# GWHB image layout

512-byte Universal Homebrew Header prefix for proc's generic loader
([PR #96](https://github.com/sylverb/game-and-watch-retro-go-sd/pull/96)).

Built via **`make cupcake-bin`** → single **`release/cupcake.bin`**.

| File | Role |
|------|------|
| `gwhb.ld` | Header at 0, entry at +512 (link-time ASSERTs) |
| `gwhb_header.c` | `gwhb_header_t` instance at image offset 0 |
| `gwhb_entry.c` | Entry at +512 — LCD RGB565, BSS zero, `app_main_cupcake` |
| `gwhb.h` | Header struct (sync with upstream firmware) |

Shared game code and ABI stubs live in [`../gnw/`](../gnw/).
