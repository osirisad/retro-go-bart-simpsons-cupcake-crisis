# Game logic (from RetroFab `build.js`)

Source: `AcclaimCupcakeCrisis` and playables `$P.Bart`, `$P.Cupcakes`, etc. in `docs/AcclaimCupcakeCrisis.js`.

## Modes

| Mode | Behavior |
|------|----------|
| `demo` | Attract; scoreboard shows hi-score; `enabled=false` |
| `start` | 3.16s timer, 2 ticks: show phase/level, play `start` sound, spawn entities |
| `play` | Main loop; `enabled=true` |
| `over` | Game over sound; wait for Select → `CON` on scoreboard |

## Input

See **`docs/INPUT_MAPPING.md`** for the full G&W / retro-go / SDL bit layout.

- **Move** (L/R/U/D): `bart.move(direction)` when `enabled`
- **Action**: If scoreboard shows `CON` → `onStart(phase)`; else in demo with level set → `onStart(1)`; else in play → `bart.action()` (throw toward Marge)
- **Select**: In demo cycles `level` 0→1→2→0; at level 0 returns to demo; after over sets `CON`
- **Level1 / Level2**: Quick start that level

## Level vs phase (easy to confuse)

The scoreboard has **one** text slot (`L-N` or `P-N`); setting phase overwrites level.

| Term | Meaning | When set |
|------|---------|----------|
| **Level** 1 or 2 | Difficulty preset (Beginner / Advanced) — speed tables | Demo Select, quick-start keys, start intro (`onStart(phase, true)`) |
| **Phase** 1–6 | In-run progression within a single game | After first phase complete, CON continue, start intro without level |

**Itch.io “level 1 done → music → level 2”** is almost always **phase 1 → phase 2**: at 10,000 points `onPhaseComplete` plays `phase.mp3` (~4s), then `onPhaseRestart` resumes play at the next phase automatically — no button press. Difficulty **Level 2** (Advanced) is only chosen before a run (Select / key `2`); JS never bumps `level` from 1→2 mid-game.

## Phases & scoring

- Phases 1–6 per run; `phasethreshold` default **10000** points per phase (cheat/debug uses 1000).
- `addPoints(n)` updates score; when `points >= phase * phasethreshold` → `onPhaseComplete`.
- Phase complete: pause, play `phase` sound (~4s), increment phase, maybe reduce miss count, `onPhaseRestart` (0.75s pause, then auto-resume — no button press).

## Level speed (timer `game` rate)

Level 1 phase rates (seconds per tick, index = phase):  
`[0, 26/30, 23/30, 21/30, 19/30, 17/30, 15/30]`

Level 2:  
`[0, 19/30, 17/30, 15/30, 13/30, 11/30, 9/30]`

When near phase threshold (90%+), rate reduced by 15%.

## Bart (`$P.Bart`) — position picks **which** sprite, not translation

Bart is a `$m.DS` (display sprite list): `bart0`…`bart9`. The playfield has positions **0–5**. Changing `bart.position` does **not** move one bitmap; it toggles **material.visible** on fixed mesh slots (same as RetroFab / `Spritesheet.show`).

From `Bart.position` setter in `AcclaimCupcakeCrisis.js`:

| Position | Bart materials shown | Notes |
|----------|----------------------|--------|
| 0 | `bart0`, `bart1` | Hand-off to Marge (`action()` animates here) |
| 1 | `bart1`, `bart7` | Far-left lane; `bart7` = arm up/down variant |
| 2 | `bart2` | Lane 2 |
| 3 | `bart3` | Lane 3 |
| 4 | `bart4`, `bart8` | Lane 4 |
| 5 | `bart5`, `bart8` | Couch sit (`sit()`) |

Each `bartN` mesh already includes the plate and **up to five cupcake slots** baked into that pose on the atlas. The port must **not** slide a single `cake04` under Bart.

Other behavior:

- **Left/Right**: `position` 1–4.
- **Up/Down** at 4 + couch on screen → `sit()` → position 5.
- **action()** at 1: timer → position 0 → `marge.collect()`.
- **catchCupcake**: `count++` (max 5), +100, sound `cupcake`.
- **catchPacifier**: +300 at position 2.

## Cupcakes grid (`$P.Cupcakes`) — show/hide stack slots, not free placement

`GridSprite` with **4 lanes × 5 stack slots** (names from `sprites-color.png` rows):

| Lane (game index 1–4) | Slot 1 | 2 | 3 | 4 | 5 |
|-----------------------|--------|---|---|---|---|
| 1 | cake01 | cake02 | cake03 | cake04 | cake05 |
| 2 | cake11 | cake12 | cake13 | cake14 | cake15 |
| 3 | cake21 | cake22 | cake23 | cake24 | cake25 |
| 4 | cake31 | cake32 | cake33 | cake34 | cake35 |

(The JS grid array also has a fifth row `cake41`–`cake45` for `cupcakes[4]` when `Math.min(4, position)` hits index 4 at the rightmost lane; the port uses the same 4×5 table as rows 1–4 above for lanes 1–4.)

Names are **not** computed positions — each is its own material at a fixed mesh/LCD location from `sprites.json`.

When Bart moves to lane `c` with `count` cupcakes, the game sets:

```js
for (var b = 1; b <= a.count; b++)
  cupcakes[Math.min(4, c)][b].visible = true;
```

So lane 3 with 2 caught cupcakes shows **`cake31` + `cake32` only** — same LCD positions every time. Miss logic hides `cupcakes[a][1]` when Bart is not in lane `a` (and not on couch).

**Aircakes** (`cake0`…`cake9`) are separate flying sprites; landing fills the grid or triggers catch/miss.

## What the C port should do

| Mode | Renderer |
|------|----------|
| **Demo** | Replay `demo.model` name lists (material visibility) — correct approach. |
| **Play** | Implement Bart `position` setter + cupcake grid visibility; **do not** offset `bart2` or duplicate `cake04` at computed x/y. |

## Aircakes (`$P.Aircakes`)

Sprites `cake0`…`cake9` — flying cupcakes. `step()` moves them through a scripted pattern; landing creates grid cupcake or catch/miss at Bart’s lane.

## Maggie

Cycles `maggie0`–`maggie3`; throws when couch slot 3 not visible; at loop 3 shows `aircakes[8]` and schedules reset.

## Marge

Appears when Bart has cupcakes and timer conditions met; `collect()` at position 0 runs bonus counter delivering held cupcakes (+100 each, +500 if 5 cupcakes).

## Couch

Random spawn after counter; timer animates `couch0`–`couch4`; ends in `onM_Couch` miss sequence unless Bart sits for bonus.

## Pacifier

Two sprites; bonus +300 and restart pacifier timer.

## Demo replay

`game/demo.model` lists per-frame visible sprite name arrays — use as integration test once renderer exists.
