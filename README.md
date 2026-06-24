# Bart Simpson's Cupcake Crisis — Retro-Go port

Standalone C port of the [RetroFab Acclaim SuperPlay simulation](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis) for [retro-go](https://github.com/ducke1937/retro-go) handhelds, with an SDL2 build for PC development.

## Quick start

**Requirements:** gcc, SDL2, make. On Windows see [docs/BUILD_WINDOWS.md](docs/BUILD_WINDOWS.md).

```bash
make
make run
make test          # or: make test-all — 40+ unit tests
```

**Dev shortcuts:** `run-pc.bat --start 1,1,9900` (jump near phase threshold); Alt+1…6 in play jumps phase (PC debug build).

**Controls:** ←→↑↓ move, **Z** action, **X** select. See [docs/INPUT_MAPPING.md](docs/INPUT_MAPPING.md).

## Assets (local only)

Game assets are not in the repo. Extract from an itch.io HAR capture:

```bash
python tools/extract_har.py path/to/capture.har
python tools/gen_sprites.py
make && make run
```

Details: [docs/HAR.md](docs/HAR.md).

## Documentation

| Doc | Contents |
|-----|----------|
| [docs/BUILD.md](docs/BUILD.md) | PC + retro-go builds, tests, asset pipeline |
| [docs/BUILD_WINDOWS.md](docs/BUILD_WINDOWS.md) | MSYS2 setup on Windows |
| [docs/PORTING.md](docs/PORTING.md) | Architecture, Celeste comparison, port phases |
| [docs/GAME_LOGIC.md](docs/GAME_LOGIC.md) | Game rules and state machine |
| [docs/INPUT_MAPPING.md](docs/INPUT_MAPPING.md) | Buttons across JS, PC, and device |
| [docs/SPRITES.md](docs/SPRITES.md) | Sprite atlas and UV mapping |
| [docs/HAR.md](docs/HAR.md) | HAR contents and extraction |
| [docs/HOST_WEB.md](docs/HOST_WEB.md) | Running the original in a browser |
| [docs/PARITY_CHECKLIST.md](docs/PARITY_CHECKLIST.md) | Manual itch.io parity sign-off |
| [SPRINT_BOARD.md](SPRINT_BOARD.md) | Development progress |

## Legal

Personal/educational use only. See [docs/LEGAL.md](docs/LEGAL.md).
