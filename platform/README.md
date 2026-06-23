# Platform hosts

| Directory | Role |
|-----------|------|
| `sdl/` | PC test binary — full bezel (`screen.jpg`) + LCD sprite layer |
| `retrogo/` | G&W `linux` emulator — RGB565 320×240, odroid save states |
| `host_draw.c` | Shared atlas → LCD blit (used by both) |
| `stb/` | stb_image for PNG/JPG without SDL_image |

Only **`platform/sdl/main.c`** and **`platform/retrogo/main.c`** differ; keep game logic in `src/cupcake_game.c`.
