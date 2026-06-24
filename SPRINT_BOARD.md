# Cupcake Crisis C Port — Sprint Board

Gap analysis comparing the original JavaScript game (`assets/build.js`, logic slice in `docs/AcclaimCupcakeCrisis.js`) against the current C port (`src/`).

**Legend — JS source of truth**

| Area | JS reference | C implementation today |
|------|----------------|------------------------|
| Game shell | `AcclaimCupcakeCrisis` prototype | `cupcake_game.c` — partial |
| Entities | `$P.Bart`, `Cupcakes`, `Aircakes`, `Maggie`, `Marge`, `Couch`, `Pacifier`, `M_` | Draw helpers only; no simulation |
| Demo | `demo.model` → `cupcake_demo.h` | Replay works (~163 frames @ 0.25s) |
| Sprites | `sprites.json` + atlas | `cupcake_sprites.h`, masks, LCD positions — generated |
| Audio | `game.model` sounds block (17 clips) | SDL_mixer (PC + retro-go emu); odroid PCM mixer on device |
| Scoreboard | `scoreboard.*` + `digit**` materials | Unified `cupcake_scoreboard_draw()` — demo/start/play/over + CON overlay |
| Hosts | N/A | SDL + retro-go shells load atlas/bezel |

**Modes in JS vs C**

| Mode | JS | C |
|------|----|---|
| `demo` | Hi-score on scoreboard, level 0, Select cycles 1→2→0 | Demo replay + hi-score digits + Select level cycle |
| `start` | 3.16s timer, 2 ticks, spawn entities | Intro via `cupcake_on_start()` before play |
| `play` | Full entity tick loop | Bart lane move; Action → `cupcake_bart_action()` stub |
| `over` | `over` sound, wait Select → `CON` | Third miss → over; Select → CON in demo |

## Sprint roadmap

| Sprint | Theme | Tasks |
|--------|-------|-------|
| 1 | Core infrastructure | TASK-01 – TASK-03 |
| 2 | Game mode state machine | TASK-04 – TASK-06 |
| 3 | Input, demo UX & wiring fixes | TASK-07, TASK-47, TASK-48, TASK-53, TASK-57, TASK-58 |
| 4 | Scoreboard & scoring | TASK-08, TASK-09, TASK-10, TASK-11, TASK-12, TASK-13, TASK-54, TASK-55 |
| 5 | Phase lifecycle & game tick | TASK-14, TASK-15, TASK-52 |
| 6 | Bart entity | TASK-16, TASK-17, TASK-18, TASK-19, TASK-20, TASK-21, TASK-22, TASK-49 |
| 7 | Cupcakes grid & aircakes | TASK-23, TASK-24, TASK-25, TASK-26 |
| 8 | Maggie, Marge, couch & pacifier | TASK-27, TASK-28, TASK-29, TASK-30, TASK-31, TASK-32 |
| 9 | Miss system | TASK-33, TASK-34, TASK-35, TASK-36, TASK-56 |
| 10 | Sprites & play renderer | TASK-61, TASK-37, TASK-45, TASK-46, TASK-50 |
| 11 | Audio | TASK-38, TASK-39, TASK-40, TASK-41 |
| 12 | Platform, QA & ship | TASK-42, TASK-43, TASK-44, TASK-51, TASK-59, TASK-60 |
| 13 | Bug fixes & parity polish | TASK-62 |

## Sprint 1 — Core infrastructure

Foundation layer everything else builds on: timed sequences need the timer module; entity spawns need RNG; modes and entities need a complete state struct.

| Task | Title | Status |
|------|-------|--------|
| TASK-01 | Portable timer subsystem | [Done - Sprint 1] |
| TASK-02 | Seeded/random helper matching `$G.random` | [Done - Sprint 1] |
| TASK-03 | Expand core game state struct for all entities | [Done - Sprint 1] |

---

## Sprint 1

[TASK-01] Portable timer subsystem

    Type: Feature

    Status: [Done - Sprint 1]

    Acceptance Criteria: C module implements JS `$G.Timer` semantics used by the game: configurable `rate` (fixed float or callback), `ticks`, `onStart`, per-tick `OT`, `onEnd`, `start` offset, named timer stop, `pause`/`resume` on a timer group, and `schedule(delay, fn)` deferred callbacks. All game sequences in `AcclaimCupcakeCrisis.js` (`start`, `game`, `phase`, `miss`, `action`, `bonus`, `couch`, `pacifier`) can be expressed without ad-hoc frame counters.

    Work Summary: Added `src/cupcake_timer.h` / `cupcake_timer.c` (`$G.Timer` + `$G.Timers` parity): named slots (`start`, `game`, `phase`, `miss`, `action`, `bonus`, `couch`, `pacifier`, `demo`), schedule pool, fixed/dynamic rate, `start_tick`, `delay_sec`, `max_ticks`, `on_start`/`on_tick`/`on_end`, group pause/resume/stop (respects `persist`), per-slot stop. Wired into `cupcake_update()` at 30 Hz via `cupcake_timers_update()`. Exposed `cupcake_timers()` accessor. Unit tests in `test/cupcake_timer_test.c` (`make test-timer`).

    Feedback/Notes:

---

[TASK-02] Seeded/random helper matching `$G.random`

    Type: Feature

    Status: [Done - Sprint 1]

    Acceptance Criteria: Provide `cupcake_rand(n)` equivalent to JS `Math.random(n)` used by `Couch.next`, `Marge.step`, and `Aircakes` branch picks. Document seed strategy (fixed seed for replay tests vs runtime seed). Couch spawn interval `20 + rand(20)` and Marge random appearance match JS distribution in manual playtesting.

    Work Summary: Added `src/cupcake_rng.h` / `cupcake_rng.c`: `cupcake_rand(n)` [0..n-1] via parseInt(Math.random()*n) semantics, `cupcake_rand_pick()` for Aircakes branch arrays, `cupcake_rand_span(base, span)` for Couch/Pacifier `base+$G.random(span)`. Seeding: `cupcake_rng_init()` at `cupcake_init()` (env `CUPCAKE_RNG_SEED`, optional compile-time `CUPCAKE_RNG_FIXED_SEED`, else time()+address mix); `cupcake_rng_seed()` / get/set state for tests and future save blobs. Unit tests in `test/cupcake_rng_test.c` (`make test-rng`).

    Feedback/Notes:

---

[TASK-03] Expand core game state struct for all entities

    Type: Feature

    Status: [Done - Sprint 1]

    Acceptance Criteria: `cupcake_state_t` (or sub-structs) holds per-entity runtime fields from JS: cupcake grid visibility (4×5 + lane-5 row), aircake slot visibility (cake0–cake9), maggie loop/index, marge onscreen/loop, couch counter/next/onscreen/anim index, pacifier counter/next/loop, miss count, scoreboard fields, active timer handles/phase, `enabled` flag, and mode. `cupcake_state_size` / save / load remain correct after expansion.

    Work Summary: Added `src/cupcake_state.h` / `cupcake_state.c` with `cupcake_play_state_t` entity sub-structs (bart, grid 5×5 bits, aircakes, maggie, marge, couch, pacifier, miss, scoreboard w/ addBonus fields) plus `cupcake_state_t` (play + demo + RNG + timer snapshots). Save blob v1 (`CUPCAKE_SAVE_MAGIC`) exports timers/RNG; `cupcake_load_state()` returns 0 on bad magic/version. Public accessors: `cupcake_get_state()`, `cupcake_play_state()`. Grid/aircake bit helpers; `cupcake_play_state_reset/start_demo/start_phase`. Tests: `test/cupcake_state_test.c` (`make test-state`).

    Feedback/Notes:

---

## Sprint 2 — Game mode state machine

Runs on Sprint 1 infrastructure (timers, RNG, state). Implements the JS mode shell: intro → play → game over, plus pause/resume used by miss and bonus sequences.

| Task | Title | Status |
|------|-------|--------|
| TASK-04 | Implement `start` mode sequence | [Done - Sprint 2] |
| TASK-05 | Implement `over` mode and Select → CON flow | [Done - Sprint 2] |
| TASK-06 | Wire `pause` / `resume` during miss and bonus sequences | [Done - Sprint 2] |

---

## Sprint 2

[TASK-04] Implement `start` mode sequence

    Type: Feature

    Status: [Done - Sprint 2]

    Acceptance Criteria: Transition `demo` → `start` via `onStart` paths (Action with level set, Quick Start, CON continue). Run 3.16s timer with 2 ticks: tick 1 shows level hi-score on scoreboard; tick 2 resets run score, calls `onPhaseStart`. Play `start` SFX. Reset Bart (position 2, count 0), start Maggie and miss counter. Matches `AcclaimCupcakeCrisis.onStart` in JS.

    Work Summary: `cupcake_on_start()` drives `CUPCAKE_TMR_START` (3.16s × 2 ticks) with entity resets on tick 0, hi-score on tick 1, `onPhaseStart` on tick 2. Demo Action (level set) enters start intro instead of play. Entity `start()` helpers in `cupcake_state.c`; `make test-start`.

    Feedback/Notes:

---

[TASK-05] Implement `over` mode and Select → CON flow

    Type: Feature

    Status: [Done - Sprint 2]

    Acceptance Criteria: On third miss, enter `over` mode: stop timers/sounds, play `over` SFX, clear Bart cupcake count, disable input except Select. Select sets scoreboard text to `CON` and returns to `demo`. Action with `CON` visible restarts at current phase per JS `onAction`.

    Work Summary: `cupcake_on_game_over()` stops timers/SFX, plays `over`, clears Bart count, sets `CUPCAKE_MODE_OVER`. Select in over → `show_con` + demo. Action with CON → `cupcake_on_start(phase)`. `cupcake_miss_increase()` triggers game over at 3 misses. `make test-over`.

    Feedback/Notes:

---

[TASK-06] Wire `pause` / `resume` during miss and bonus sequences

    Type: Feature

    Status: [Done - Sprint 2]

    Acceptance Criteria: `enabled` false while paused; game timer group paused during `onM_Cupcake`, `onM_Couch`, couch sit bonus, Marge `addBonus`, and phase-complete interstitials. `resume` restores play identically to JS.

    Work Summary: `cupcake_pause()` / `cupcake_resume()` mirror JS (`timers` game-slot pause + `enabled`). Wired into `onM_Cupcake`, `onM_Couch`, `onPhaseComplete`, `onPhaseRestart` (0.75s interstitial), `bart_sit_bonus`, and `scoreboard_add_bonus`. Minimal game timer OT in `onPhaseStart`. `make test-pause`.

    Feedback/Notes:

---

## Sprint 3 — Input, demo UX & wiring fixes

Routes buttons correctly; fixes demo→play shortcuts; enables Quick Start and enabled-flag gating before entity sim lands.

| Task | Title | Status |
|------|-------|--------|
| TASK-07 | Full input router (`onPress` parity) | [Done - Sprint 3] |
| TASK-47 | Fix demo start bypassing `start` mode | [Done - Sprint 3] |
| TASK-48 | Fix play-mode Action debug cupcake increment | [Done - Sprint 3] |
| TASK-53 | `onQuickStart` Level1 / Level2 | [Done - Sprint 3] |
| TASK-57 | Integrate `enabled` flag with input and ticks | [Done - Sprint 3] |
| TASK-58 | Host input mapping documentation and retro-go buttons | [Done - Sprint 3] |

---

## Sprint 3

[TASK-07] Full input router (`onPress` parity)

    Type: Feature

    Status: [Done - Sprint 3]

    Acceptance Criteria: Map host buttons to JS handlers: Left/Right/Up/Down → `onMove` (only when `enabled`), Action → `onAction`, Select → `onSelect`, Level1/Level2 → `onQuickStart`. Remove play-mode debug hack where Action increments cupcake count. SDL host exposes Level1/Level2 keys (e.g. `1`/`2`).

    Work Summary: `cupcake_handle_input()` mirrors JS `onPress`: move/select/action/quick-start routed through `cupcake_on_move/select/action/quick_start`. Added `CUPCAKE_BTN_LEVEL1/2`; SDL + retro-go map keys `1`/`2`. Play Action debug increment removed. `make test-input`.

    Feedback/Notes: 

---

[TASK-47] Fix demo start bypassing `start` mode

    Type: Bug Fix

    Status: [Done - Sprint 3]

    Acceptance Criteria: Starting a level from demo enters `CUPCAKE_MODE_START` (3.16s intro) before `play`, not direct `CUPCAKE_MODE_PLAY` as current `cupcake_update` does.

    Work Summary: Fixed in TASK-04/07: demo Action (`onStart(1)`) and Level1/2 quick start route through `cupcake_on_start()` → start timer → `onPhaseStart` → play. Regression tests in `make test-demo-start`.

    Feedback/Notes: 

---

[TASK-48] Fix play-mode Action debug cupcake increment

    Type: Bug Fix

    Status: [Done - Sprint 3]

    Acceptance Criteria: Remove temporary Action handler that cycles `bart_cupcakes` and awards free points. Action in play delegates to Bart `action()` / start / CON only.

    Work Summary: Removed debug increment (TASK-07). `cupcake_on_action()` now routes play+enabled to `cupcake_bart_action()` stub (TASK-19); CON/demo paths unchanged. `make test-action`.

    Feedback/Notes: 

---

[TASK-53] `onQuickStart` Level1 / Level2

    Type: Feature

    Status: [Done - Sprint 3]

    Acceptance Criteria: Level1/Level2 inputs stop simulation, set level 1 or 2, call `onStart(1, true)` showing level on scoreboard (not phase). Works from demo/attract.

    Work Summary: `cupcake_on_quick_start()` stops timers, sets level/scoreboard.level, calls `onStart(1, show_level=1)`. Routed from `cupcake_handle_input()` via `CUPCAKE_BTN_LEVEL1/2` (SDL/retro-go keys `1`/`2`). Start intro `on_start` sets scoreboard level digit instead of phase. `make test-quick-start`.

    Feedback/Notes: 

---

[TASK-57] Integrate `enabled` flag with input and ticks

    Type: Feature

    Status: [Done - Sprint 3]

    Acceptance Criteria: Movement and game tick OT callbacks run only when `enabled` true (play mode, not paused). Demo and over modes ignore move input per JS.

    Work Summary: Central `cupcake_play_active()` gates `on_move`, play Action, and `tmr_game_ot`. Pause/resume (TASK-06) pauses game timer slot + clears `enabled`; demo/start/over clear `enabled` via `on_stop` or mode entry. `make test-enabled`.

    Feedback/Notes: 

---

[TASK-58] Host input mapping documentation and retro-go buttons

    Type: Feature

    Status: [Done - Sprint 3]

    Acceptance Criteria: `docs/` table lists physical G&W / retro-go button mapping to `CUPCAKE_BTN_*`. retro-go `main.c` maps device inputs to same bit layout as SDL.

    Work Summary: Added `docs/INPUT_MAPPING.md` (JS, G&W physical, bit index, SDL, retro-go device + linux emu). Shared `platform/cupcake_input.c` (`cupcake_input_from_sdl_keyboard`, `cupcake_input_from_odroid`). SDL + retro-go hosts use same `CUPCAKE_BTN_*` layout; retro-go `LINUX_EMU` uses SDL keys (Z/X/1/2/WASD), device firmware uses odroid A/B/dpad/START/X.

    Feedback/Notes: 

---

## Sprint 4 — Scoreboard & scoring

Digit renderer, hi-scores, addBonus animation, phase/level indicators, and core addPoints/phase threshold logic.

| Task | Title | Status |
|------|-------|--------|
| TASK-08 | Demo mode scoreboard hi-score display | [Done - Sprint 4] |
| TASK-09 | Demo Select level cycle with `stop()` semantics | [Done - Sprint 4] |
| TASK-10 | Scoreboard digit renderer | [Done - Sprint 4] |
| TASK-11 | Animated `addBonus` counter | [Done - Sprint 4] |
| TASK-12 | Hi-score persistence | [Done - Sprint 4] |
| TASK-13 | `addPoints` and phase threshold scoring | [Done - Sprint 4] |
| TASK-54 | Phase indicator on scoreboard during play | [Done - Sprint 4] |
| TASK-55 | Level indicator on scoreboard | [Done - Sprint 4] |

---

## Sprint 4

[TASK-08] Demo mode scoreboard hi-score display

    Type: Feature

    Status: [Done - Sprint 4]

    Acceptance Criteria: In `demo`, scoreboard shows persisted hi-score (5 digits, `alwayson` style per JS `scoreboard.start`). Level indicator hidden or 0. Visual output uses `digit**` sprites from atlas. No stderr-only feedback for level selection.

    Work Summary: Added `cupcake_scoreboard.c` — JS 7-segment bitmask → `digit{col}{seg}` sprites; demo attract draws 5-digit hi-score (`alwayson=2`). `cupcake_play_state_start_demo()` / `cupcake_on_demo()` sync `scoreboard.value` from `hi_score[0]`; demo update keeps value pinned. Wired into `cupcake_draw()` demo path. `make test-demo-scoreboard`.

    Feedback/Notes: 

---

[TASK-09] Demo Select level cycle with `stop()` semantics

    Type: Feature

    Status: [Done - Sprint 4]

    Acceptance Criteria: Select in demo increments level 0→1→2→0. Non-zero level stops sounds/timers, keeps demo mode, sets `scoreboard.level`. Level 0 restores hi-score display and full demo replay. Matches `onSelect` demo branch.

    Work Summary: `cupcake_on_select()` mirrors JS: level>0 calls `cupcake_stop()`, keeps demo, sets `scoreboard.level`; level 0 restores `scoreboard.value` from hi-score and `cupcake_on_demo()`. Demo replay frozen when level selected (`SS.on=false`); scoreboard shows `L-1`/`L-2` via `cupcake_scoreboard_draw_demo_level()`. Removed stderr level feedback. `make test-demo-select`.

    Feedback/Notes: 

---

[TASK-10] Scoreboard digit renderer

    Type: Feature

    Status: [Done - Sprint 4]

    Acceptance Criteria: Render 5-digit scores and smaller phase/level indicators using `digit00`–`digit46` materials from `cupcake_sprites.h`. API supports `score`, `value`, `phase`, `level`, and text overlay `CON`. Positions match LCD mesh layout (verify against browser screenshot).

    Work Summary: Unified `cupcake_scoreboard_draw()` plus `draw_digits` / `draw_text` APIs — 7-segment C/O/N/P/L/- chars on columns 4..2; 5-digit `score`/`value` on columns 0..4. Wired into demo, start, play, and over in `cupcake_draw()`. Start intro clears stale level/phase overlay fields. `make test-scoreboard`.

    Feedback/Notes: 

---

[TASK-11] Animated `addBonus` counter

    Type: Feature

    Status: [Done - Sprint 4]

    Acceptance Criteria: Port JS `scoreboard.addBonus({rate, points, increment, OT, onEnd})` used by Marge delivery and couch sit bonus. Points tick up with `points` SFX; triggers `onScoreChange` each increment. Five-cupcake delivery plays `five` on first tick when applicable.

    Work Summary: `cupcake_scoreboard_add_bonus_ex()` in `cupcake_scoreboard.c` — bonus timer on `CUPCAKE_TMR_BONUS`, pause/resume via host hooks, per-tick points/score sync + `onScoreChange` (phase threshold). Flags `play_points_sfx` / `play_five_on_first` for couch vs Marge five-delivery rules; optional `on_tick`/`on_end` callbacks. Couch path uses `cupcake_scoreboard_add_bonus()` wrapper; `cupcake_add_points()` for direct scoring. `make test-add-bonus`.

    Feedback/Notes: 

---

[TASK-12] Hi-score persistence

    Type: Feature

    Status: [Done - Sprint 4]

    Acceptance Criteria: Per-level hi-scores stored and loaded (JS `$G.$st.hiscores[level]`). Updated on game over / score change per `AcclaimSuperplayDevice.onScoreChange`. PC saves to user-writable file; retro-go saves beside other emu saves.

    Work Summary: Added `cupcake_hiscore.c` — binary `HISC` v1 file with 3 level scores; load on `cupcake_init()`, save when beaten via `cupcake_hiscore_on_score_change()` (wired into score-change path + game over). PC default `%LOCALAPPDATA%\\cupcake_hiscores.dat` (override `CUPCAKE_HISCORE_PATH`); retro-go uses `$CUPCAKE_ASSETS/cupcake_hiscores.dat`. `make test-hiscore`.

    Feedback/Notes: 

---

[TASK-13] `addPoints` and phase threshold scoring

    Type: Feature

    Status: [Done - Sprint 4]

    Acceptance Criteria: `addPoints(n)` updates run score and scoreboard. When `points >= phase * phasethreshold` (default 10000), trigger `onPhaseComplete`. Phase threshold configurable via `setThreshold` (1000 debug / 10000 default).

    Work Summary: `cupcake_add_points()` syncs `points`/`scoreboard.score` and calls `onScoreChange` (hi-score + phase check). `cupcake_set_threshold()` / `cupcake_phase_score_target()` mirror JS `setThreshold`; default 10000 on init and `onStart`. Phase complete guarded to play mode and single in-flight `CUPCAKE_TMR_PHASE`. `make test-add-points`.

    Feedback/Notes: 

---

[TASK-54] Phase indicator on scoreboard during play

    Type: Feature

    Status: [Done - Sprint 4]

    Acceptance Criteria: `scoreboard.phase` shows clamped phase 1–6 during start/play/phase transitions. Updates on `onPhaseRestart` and phase complete.

    Work Summary: `cupcake_scoreboard_set_phase()` clamps and assigns `scoreboard.phase`; synced at `onStart`, start timer onStart, `onPhaseStart`, and `onPhaseRestart`. `P-{phase}` overlay drawn in start/play/over via existing scoreboard renderer. `make test-phase-indicator`.

    Feedback/Notes: 

---

[TASK-55] Level indicator on scoreboard

    Type: Feature

    Status: [Done - Sprint 4]

    Acceptance Criteria: Show level 1 or 2 on scoreboard when selected in demo or during quick start / play. Hidden or 0 in attract per JS.

    Work Summary: `cupcake_scoreboard_set_level()` sets `scoreboard.level` (0=hidden, 1..2=L-N). Wired into demo Select, `onDemo` restore, quick start, and start intro (`show_level`). Level overlay takes priority over phase in `draw_overlay`; attract shows hi-score only. `make test-level-indicator`.

    Feedback/Notes: 

---

## Sprint 5 — Phase lifecycle & game tick

Phase complete/restart flow and the main game timer that drives all entity step() calls.

| Task | Title | Status |
|------|-------|--------|
| TASK-14 | Phase complete and restart flow | [Done - Sprint 5] |
| TASK-15 | Game tick timer with level/phase speed tables | [Done - Sprint 5] |
| TASK-52 | Record-mode timing constants | [Done - Sprint 5] |

---

## Sprint 5

[TASK-14] Phase complete and restart flow

    Type: Feature

    Status: [Done - Sprint 5]

    Acceptance Criteria: `onPhaseComplete`: pause, 4s timer, stop sounds, play `phase` SFX, increment phase (max 6), optionally `miss.decrease()`, call `onPhaseRestart`. `onPhaseRestart`: update scoreboard phase, stop timers, restart phase entities, 0.75s delay, sync score display, restart pacifier, resume. Matches JS ordering.

    Work Summary: Refactored `cupcake_on_phase_start()` to JS entity order (bart, maggie, cupcakes, aircakes, pacifier, marge, couch). Phase complete uses 4s `CUPCAKE_TMR_PHASE` with stop/phase SFX; end increments/clamps phase, `cupcake_miss_decrease()`, then `onPhaseRestart`. Restart: scoreboard phase, stop timers, phase start, pause, stop SFX, 0.75s schedule → score sync, pacifier, resume. `make test-phase-complete`.

    Feedback/Notes: 

---

[TASK-15] Game tick timer with level/phase speed tables

    Type: Feature

    Status: [Done - Sprint 5]

    Acceptance Criteria: `timers['game']` rate from JS tables: Level 1 `[0, 26/30, 23/30, 21/30, 19/30, 17/30, 15/30]` sec/tick; Level 2 `[0, 19/30, 17/30, 15/30, 13/30, 11/30, 9/30]`. When score ≥ 90% of phase threshold and < 100%, reduce rate by 15%. Each tick calls `cupcakes.step`, `aircakes.step`, `maggie.step`, `marge.step(tick)`, `couch.step(tick)`, `pacifier.step`.

    Work Summary: `cupcake_game_tick_rate_sec()` mirrors JS level/phase tables and debug-threshold slowdown. Game timer uses dynamic `rate_fn`; `tmr_game_ot` calls entity `step()` stubs (pacifier/couch decrement `next`). Timer restore re-binds rate + OT. `make test-game-tick`.

    Feedback/Notes: 

---

[TASK-52] Record-mode timing constants

    Type: Feature

    Status: [Done - Sprint 5]

    Acceptance Criteria: When record flag set (if ported), couch `next=25` and pacifier `next=15` per JS. Default record flag off.

    Work Summary: `cupcake_set_record()` / `cupcake_record_mode()` on play state (default off, preserved across `cupcake_init`). `cupcake_pacifier_start` / `cupcake_couch_start` use fixed `next` when record set, else `20+rand(20)`. `make test-record-mode`.

    Feedback/Notes: 

---

## Sprint 6 — Bart entity

Full Bart simulation: move, catch, throw, sit, position-based draw, miss poses.

| Task | Title | Status |
|------|-------|--------|
| TASK-16 | Bart `start` and initial spawn state | [Done - Sprint 6] |
| TASK-17 | Bart `move` — lane changes and grid pickup | [Done - Sprint 6] |
| TASK-18 | Bart Up/Down `sit` on couch | [Done - Sprint 6] |
| TASK-19 | Bart `action` throw toward Marge | [Done - Sprint 6] |
| TASK-20 | Bart `catchCupcake` and `catchPacifier` | [Done - Sprint 6] |
| TASK-21 | Bart position setter — material visibility parity | [Done - Sprint 6] |
| TASK-22 | Bart miss pose sprites (`bart6`, `bart9`) | [Done - Sprint 6] |
| TASK-49 | `draw_bart_position(0)` layer correctness | [Done - Sprint 6] |

---

## Sprint 6

[TASK-16] Bart `start` and initial spawn state

    Type: Feature

    Status: [Done - Sprint 6]

    Acceptance Criteria: `bart.start(2)` resets cupcake count to 0 and sets position 2 at phase start. All Bart materials hidden except those required by position setter.

    Work Summary: `cupcake_bart_set_position()` ports JS material visibility (bart0–bart9 bitmask, lane stack hide/show). `cupcake_bart_start()` resets count/miss and applies position. Play draw uses visibility + grid stack. `make test-bart-start`.

    Feedback/Notes: 

---

[TASK-17] Bart `move` — lane changes and grid pickup

    Type: Feature

    Status: [Done - Sprint 6]

    Acceptance Criteria: Left/Right moves positions 1–4. Moving into a lane with bottom grid cupcake (`cupcakes[lane][1]` visible) catches if count < 5 (hide cell, `catchCupcake`) or triggers `onM_Cupcake` if count == 5. Pacifier at lane 2 calls `catchPacifier`. Marge `collect()` side effects on move. Plays `move` SFX on normal move.

    Work Summary: `cupcake_bart_move()` mirrors JS move order (pacifier catch, `cupcake_marge_collect` count rule, grid pickup/miss, `set_position`). Catch helpers add points/SFX; full Marge delivery bonus deferred to TASK-29. `make test-bart-move`.

    Feedback/Notes: 

---

[TASK-18] Bart Up/Down `sit` on couch

    Type: Feature

    Status: [Done - Sprint 6]

    Acceptance Criteria: At position 4 with couch on screen, Up or Down calls `sit()`: position 5, pause game, 2s bonus timer, `bonus` SFX, award points by visible couch frame (couch3→100, couch2→200, couch1→400), restart couch timer, resume. Matches JS `Bart.sit`.

    Work Summary: `cupcake_bart_sit()` on Up/Down at lane 4 + couch onscreen; `frame_visible` bitmask drives `cupcake_couch_sit_bonus_ticks()`. Existing 2s `CUPCAKE_TMR_ACTION` path plays bonus SFX, restarts couch, resumes, runs addBonus. Couch4 skips bonus timer (no soft-lock). `make test-bart-sit`.

    Feedback/Notes: 

---

[TASK-19] Bart `action` throw toward Marge

    Type: Feature

    Status: [Done - Sprint 6]

    Acceptance Criteria: At position 1, Action runs timed sequence: animate to position 0, call `marge.collect()`, return to position 1 with count reset rules from JS. Timer rate scales with game rate and Marge on-screen (×0.5 or ×0.8).

    Work Summary: `cupcake_bart_action()` at lane 1 starts `CUPCAKE_TMR_ACTION` at `game_rate * (marge.visible ? 0.5 : 0.8)`; onStart→pos 0, onEnd→`cupcake_marge_collect()` + count 0/1 floor rule + pos 1. Timer restore distinguishes sit bonus (2s) vs throw. `make test-bart-action`.

    Feedback/Notes: 

---

[TASK-20] Bart `catchCupcake` and `catchPacifier`

    Type: Feature

    Status: [Done - Sprint 6]

    Acceptance Criteria: `catchCupcake`: increment count (max 5), +100 points, `cupcake` SFX. `catchPacifier`: +300 points, `pacifier2` SFX, restart pacifier timer. Invoked from move, aircakes landing, and pacifier step.

    Work Summary: Public `cupcake_bart_catch_cupcake()` / `cupcake_bart_catch_pacifier()`. `cupcake_aircakes_land_at_lane()` for flying-cake landing; `cupcake_pacifier_on_loop3()` for TASK-32 step hook. Move uses shared catch APIs. `make test-bart-catch`.

    Feedback/Notes: 

---

[TASK-21] Bart position setter — material visibility parity

    Type: Feature

    Status: [Done - Sprint 6]

    Acceptance Criteria: Port JS `Bart.position` setter: hide prior Bart layers and prior lane stack; show `bartN` for position N; extra layers per table (0→bart0+bart1, 1→bart1+bart7, 4/5→bart8); redraw cupcake stack slots 1..count on new lane using `Math.min(4, position)`. No sliding a single bitmap.

    Work Summary: `cupcake_bart_set_position()` (TASK-16) documents JS parity; draw uses `bart.visible` bitmask + grid stack per lane (`cupcake_bart_stack_lane`). `make test-bart-position` covers all positions, layer transitions, stack lane mapping, and move integration.

    Feedback/Notes: 

---

[TASK-22] Bart miss pose sprites (`bart6`, `bart9`)

    Type: Feature

    Status: [Done - Sprint 6]

    Acceptance Criteria: `onM_Couch` end sets Bart index 6 (`bart6`); `onM_Cupcake` sets index 9 (`bart9`). Visible during miss animations before `onM_`. Drawn instead of normal position sprites.

    Work Summary: `cupcake_bart_set_miss_pose()` / `clear_miss_pose()`. Cupcake miss OT hides grid, shows `bart9`; couch miss end hides grid/aircakes, shows `bart6`. `draw_bart_play()` skips held stack during miss. Timer restore re-binds miss OT callbacks. `make test-bart-miss`.

    Feedback/Notes: 

---

[TASK-49] `draw_bart_position(0)` layer correctness

    Type: Bug Fix

    Status: [Done - Sprint 6]

    Acceptance Criteria: Position 0 shows `bart0` + `bart1` only (hand-off pose), without erroneous duplicate rules from positions 0 and 1 sharing `bart1`. Visually matches browser at throw-to-Marge frame.

    Work Summary: Replaced ad-hoc layer rules with `cupcake_bart_position_layers()` mask table — pos 0 is `bart0|bart1` only (no `bart7` lane-1 extras). Draw uses bitmask via `draw_bart_play()`. `make test-bart-position0`.

    Feedback/Notes: 

---

## Sprint 7 — Cupcakes grid & aircakes

Grid miss logic, flying cupcake chain, and play-mode drawing for both.

| Task | Title | Status |
|------|-------|--------|
| TASK-23 | Cupcakes grid state machine | [Done - Sprint 7] |
| TASK-24 | Draw all visible grid cupcakes in play mode | [Done - Sprint 7] |
| TASK-25 | Aircakes entity — spawn and flight pattern | [Done - Sprint 7] |
| TASK-26 | Aircakes visibility draw in play mode | [Done - Sprint 7] |

---

## Sprint 7

[TASK-23] Cupcakes grid state machine

    Type: Feature

    Status: [Done - Sprint 7]

    Acceptance Criteria: 4 lanes × 5 stack slots (`cake01`–`cake05`, `cake11`–`cake15`, `cake21`–`cake25`, `cake31`–`cake35`) plus JS fifth row `cake41`–`cake45` for `Math.min(4, position)` edge case. `start()` hides grid. `step()` misses when `cupcakes[a][1]` visible and Bart not in lane `a` or couch (position 5).

    Work Summary: `cupcake_cupcakes_start()` clears grid and `group_visible`. `cupcake_cupcakes_step()` mirrors JS floor-miss loop (lanes 1–4, slot 1, skip Bart lane or couch). `cupcake_grid_set_visible(on)` restores `group_visible`. `make test-cupcakes-step`.

    Feedback/Notes: 

---

[TASK-24] Draw all visible grid cupcakes in play mode

    Type: Feature

    Status: [Done - Sprint 7]

    Acceptance Criteria: Play renderer shows every visible grid cell, not only Bart's held stack on his lane. Bottom-of-stack visibility matches JS (slot 1 miss detection; stack slots 1..n shown when caught on Bart).

    Work Summary: `cupcake_grid_draw_visible()` iterates lanes 1–5 and slots 1–5 when `group_visible`. Play draw uses it instead of Bart-lane-only held stack. `cupcake_grid_sprite_name()` adds lane-5 row `cake41`–`cake45`. `make test-cupcakes-draw`.

    Feedback/Notes: 

---

[TASK-25] Aircakes entity — spawn and flight pattern

    Type: Feature

    Status: [Done - Sprint 7]

    Acceptance Criteria: Port `$P.Aircakes.step()` chain: cake1–cake4 land or advance; cake5→1; cake6→[5,2]; cake7→[6,6,3]; cake8 phase-dependent branch; landing on Bart lane catches or misses; else sets `cupcakes[lane][1]` visible. `step` SFX on transitions. `start()` hides all.

    Work Summary: `cupcake_aircakes_step()` in `cupcake_game.c` mirrors JS chain (land 1–4, advance 5→1, random picks for 6–8 with phase≤3 vs >3 for cake8). `cupcake_aircakes_start()` hides entity; `cupcake_aircake_set_visible(on)` restores `group_visible`. `make test-aircakes-step`.

    Feedback/Notes: 

---

[TASK-26] Aircakes visibility draw in play mode

    Type: Feature

    Status: [Done - Sprint 7]

    Acceptance Criteria: Draw currently visible `cake0`–`cake9` flying sprites at LCD positions. Hidden when entity `visible` false (miss sequences). Maggie throw shows `cake8` per JS loop 3.

    Work Summary: `cupcake_aircakes_draw_visible()` draws `cake0`–`cake9` when `group_visible`. Play renderer calls it from `draw_bart_play()` (including miss cupcake when aircake0/9 shown; skipped when couch miss clears group). `make test-aircakes-draw`.

    Feedback/Notes: 

---

## Sprint 8 — Maggie, Marge, couch & pacifier

Secondary characters and hazard spawns that interact with Bart and the tick loop.

| Task | Title | Status |
|------|-------|--------|
| TASK-27 | Maggie throw cycle | [Done - Sprint 8] |
| TASK-28 | Marge appear / hide stepping | [Done - Sprint 8] |
| TASK-29 | Marge `collect` delivery bonus | [Done - Sprint 8] |
| TASK-30 | Couch spawn timer and animation | [Done - Sprint 8] |
| TASK-31 | Couch sit bonus integration | [Done - Sprint 8] |
| TASK-32 | Pacifier spawn cycle | [Done - Sprint 8] |

---

## Sprint 8

[TASK-27] Maggie throw cycle

    Type: Feature

    Status: [Done - Sprint 8]

    Acceptance Criteria: `maggie.start` / `step`: cycles `maggie0`–`maggie3` when couch slot 3 not visible. Loop 3 shows `aircakes[8]` and schedules index reset after `0.75 * game.rate`. `throw` SFX when loop > 0. Suppressed while `couch[3]` visible.

    Work Summary: `cupcake_maggie_step()` mirrors JS (index=loop, loop 3 throws cake8 + schedules index reset, throw SFX when loop>0, couch3 suppresses). Phase start calls `maggie_start()` then `maggie_step()` per JS `start(1)`. `make test-maggie-step`.

    Feedback/Notes: 

---

[TASK-28] Marge appear / hide stepping

    Type: Feature

    Status: [Done - Sprint 8]

    Acceptance Criteria: `marge.step(tick)`: when off-screen, Bart has cupcakes, tick > 6, and `(tick % 6 == 5 || tick % 6 == 0 && rand(3)==0)`, and couch not on-screen → show `marge1`, play `marge` SFX. On-screen loop hides after 2 steps. Not visible when couch on-screen.

    Work Summary: `cupcake_marge_step()` in `cupcake_game.c` mirrors JS appear/hide rules (`visible` + `loop`, couch.onscreen gate, tick%6 cadence). `make test-marge-step`.

    Feedback/Notes: 

---

[TASK-29] Marge `collect` delivery bonus

    Type: Feature

    Status: [Done - Sprint 8]

    Acceptance Criteria: When `marge1` visible, Bart at position 0, and count > 0: run `addBonus` for count (+ extra +500 if count==5), `deliver`/`five`/`points` SFX rules, `giveCupcakesToMarge`, reschedule `marge.start` after delay. Return value affects Bart action/move count reset per JS.

    Work Summary: `cupcake_marge_collect()` runs `addBonus` (rate 0.06 / 0.025, ticks count or count+5). Five-delivery tick 5 calls `giveCupcakesToMarge` + `marge.start`; normal path onEnd + 0.5×game.rate schedule. `deliver`/`five`/`points` SFX via bonus cfg. `make test-marge-collect`.

    Feedback/Notes: 

---

[TASK-30] Couch spawn timer and animation

    Type: Feature

    Status: [Done - Sprint 8]

    Acceptance Criteria: `couch.start`: counter, `next = 20+rand(20)` (25 if record mode). `step`: spawn when counter ≥ next and Marge off-screen. Animated spawn: 3s rate, 4 ticks, `couch0`–`couch4`, `couch` SFX each step; couch3 hides Maggie; end calls `onM_Couch` if Bart not sitting. `onscreen` flag exposed.

    Work Summary: `cupcake_couch_step()` increments counter and calls `couch_entity_start(p,1)` when ready. Couch spawn uses `CUPCAKE_TMR_COUCH` (3s, start_tick 1, 4 ticks) with frame/SFX OT chain and Maggie hide at couch3; `onEnd` calls `cupcake_on_m_couch()` unless Bart at lane 5. `couch_entity_start()` stops couch timer on reset/sit/phase. `make test-couch-step`.

    Feedback/Notes: 

---

[TASK-31] Couch sit bonus integration

    Type: Feature

    Status: [Done - Sprint 8]

    Acceptance Criteria: Bart `sit()` reads visible couch frame for bonus multiplier, restarts couch via `couch.start()`, does not trigger couch miss if sit before `couch4`.

    Work Summary: `cupcake_bart_sit()` stops couch spawn timer on sit; couch4 path calls `couch_entity_start()` + resume (no bonus). Bonus points read from visible couch frame at 2s timer `onEnd` (JS parity). Extended `make test-bart-sit` with sit-before-couch4 miss guard.

    Feedback/Notes:

---

[TASK-32] Pacifier spawn cycle

    Type: Feature

    Status: [Done - Sprint 8]

    Acceptance Criteria: Port `$P.Pacifier`: `start` resets counter/loop/next (`20+rand(20)`, 15 record). `step`: on interval show pacifier1 then pacifier2; loop 3 auto-catch if Bart at lane 2 else restart. `pacifier1` SFX on loop 1. `pacifier2` visible at index 2 for lane-2 catch on move.

    Work Summary: `cupcake_pacifier_step()` in `cupcake_game.c` mirrors JS counter/loop chain (3 consecutive ticks after counter≥next). `make test-pacifier-step`.

    Feedback/Notes:

---

## Sprint 9 — Miss system

Miss counter, cupcake/couch miss sequences, entity hide during miss, game-over branch.

| Task | Title | Status |
|------|-------|--------|
| TASK-33 | Miss counter (`$P.M_`) display and logic | [Done - Sprint 9] |
| TASK-34 | `onM_Cupcake` miss sequence | [Done - Sprint 9] |
| TASK-35 | `onM_Couch` miss sequence | [Done - Sprint 9] |
| TASK-36 | `onM_` branch — miss limit vs phase restart | [Done - Sprint 9] |
| TASK-56 | Hide entity groups during miss sequences | [Done - Sprint 9] |

---

## Sprint 9

[TASK-33] Miss counter (`$P.M_`) display and logic

    Type: Feature

    Status: [Done - Sprint 9]

    Acceptance Criteria: `miss.start(n)` shows `miss1`–`miss3` up to count. `increase`/`decrease` toggle sprites. Phase complete may decrease miss count. Draw miss indicators in play/over modes.

    Work Summary: `cupcake_miss_start(p, n)` mirrors JS optional initial count. `cupcake_miss_draw_visible` draws miss1..count in play/over via composite draw. `make test-miss-counter`.

    Feedback/Notes: 

---

[TASK-34] `onM_Cupcake` miss sequence

    Type: Feature

    Status: [Done - Sprint 9]

    Acceptance Criteria: Pause, 0.5s timer: play `miss` SFX, show aircake 0 or 9 based on lane, hide cupcakes, Bart `bart9`. After 1.75s schedule `onM_`. Matches JS timing.

    Work Summary: `cupcake_on_m_cupcake` + miss timer callbacks; phase restart resume via named `CUPCAKE_TMR_PHASE`. `make test-bart-miss`.

    Feedback/Notes: 

---

[TASK-35] `onM_Couch` miss sequence

    Type: Feature

    Status: [Done - Sprint 9]

    Acceptance Criteria: Pause, timer `rate 0.4`, `ticks 6 - bart.position`: force Bart toward lane 4 with `move` SFX each step; then hide cupcakes/aircakes, Bart `bart6`, `whoa` SFX, 2s delay → `onM_`.

    Work Summary: `cupcake_on_m_couch` + couch miss OT (slide to lane 5). Couch spawn end triggers miss when Bart not sitting. `make test-bart-miss`, `make test-couch-step`.

    Feedback/Notes: 

---

[TASK-36] `onM_` branch — miss limit vs phase restart

    Type: Feature

    Status: [Done - Sprint 9]

    Acceptance Criteria: `miss.increase()`; if count == 3 → `onGameOver`; else `onPhaseRestart`. Entity visibility reset consistent with JS.

    Work Summary: `cupcake_on_m_` → `cupcake_miss_increase()` branches to `cupcake_on_game_over` or `cupcake_on_phase_restart` (0.75s interstitial restores grid/aircakes).

    Feedback/Notes: 

---

[TASK-56] Hide entity groups during miss sequences

    Type: Feature

    Status: [Done - Sprint 9]

    Acceptance Criteria: JS sets `cupcakes.visible=false` and `aircakes.visible=false` during miss animations; C port equivalent hides those groups until phase restart. Pacifier/Marge/Couch timers stopped appropriately.

    Work Summary: Miss sequences hide grid (cupcake miss OT) and grid+aircakes (couch miss end). `cupcake_miss_sequence_begin` stops couch timer and hides marge/pacifier; `cupcake_pause` also pauses couch spawn timer.

    Feedback/Notes: 

---

## Sprint 10 — Sprites & play renderer

Fix multi-part sprite mapping; composite draw pass; LCD audit and alignment verification.

| Task | Title | Status |
|------|-------|--------|
| TASK-61 | Redo sprite rect/mask mapping — full material parity | [Done - Sprint 10] |
| TASK-37 | Play-mode composite draw pass | [Done - Sprint 10] |
| TASK-45 | LCD position audit — close deltas vs mesh formula | [Done - Sprint 10] |
| TASK-46 | Sprite coverage — all 96 materials blittable | [Done - Sprint 10] |
| TASK-50 | Marge1 hair LCD clamp verification | [Done - Sprint 10] |

---

## Sprint 10

[TASK-61] Redo sprite rect/mask mapping — full material parity

    Type: Bug Fix

    Status: [Done - Sprint 10]

    Acceptance Criteria: Replace largest-connected-component-only bbox logic in `tools/sprite_raster.py` / `gen_sprites.py` with a mapping that includes **all** significant detached parts per material (union of connected components above `MIN_PIX`, not only upward hair extension). Regenerate `cupcake_sprites.h`, `cupcake_sprite_masks.h`, and `cupcake_sprite_lcd.h` via `make gen`. Known regressions fixed visually: `marge1` includes “Thank you Bart” speech bubble (full mask union ~267×384, not 117×384 body-only); `pacifier1`, `maggie3`, `miss3`, `bart6`, and other multi-CC materials include all gameplay-visible parts. Every material referenced in `demo.model` or `AcclaimCupcakeCrisis.js` (`$P.*` sprite lists) has a valid rect+mask; unused JSON-only materials (e.g. `TEMP4`) documented in `docs/SPRITES.md` with exclude rationale. `test/export_sprites.py` contact sheet + `--pin` spot-checks match browser RetroFab for pinned sprites. `tools/audit_sprites.py` passes; no `host_sprite_rect_ok` failures for demo.model names. Update or supersede TASK-45 / TASK-46 / TASK-50 verification steps once this lands.

    Work Summary: `union_cc_bbox()` unions all CCs ≥ MIN_PIX in `sprite_raster.py`; `sprite_draw_bbox()` uses union + PAD. Regenerated headers (93 sprites): `marge1` 271×390, `couch4` 103×217, `bart6` 239×412, `miss3` 258×96, `pacifier1` 129×62. `export_sprites.py` aligned.

    Feedback/Notes: Root cause: JS toggles `material.visible` for all triangles in a material (e.g. single `marge1` shows body + bubble); C port cropped to largest CC. See chat analysis 2025-06.

---

[TASK-37] Play-mode composite draw pass

    Type: Feature

    Status: [Done - Sprint 10]

    Acceptance Criteria: Single play draw order reproduces visible sprite set from JS snapshot (`onSnapshot` names: bart, cake, maggie, marge, couch, pacifier). Couch, Maggie, Marge, pacifiers, grid, aircakes, Bart, miss icons, scoreboard digits layer correctly (no z-fighting; verify against browser capture).

    Work Summary: `cupcake_play_draw_visible()` in `cupcake_state.c` — draw order couch → maggie → marge → pacifier → grid → aircakes → bart → miss; couch-miss path skips grid/aircakes. `cupcake_game.c` calls it from play/start/over. `make test-play-draw`.

    Feedback/Notes:

---

[TASK-45] LCD position audit — close deltas vs mesh formula

    Type: Bug Fix

    Status: [Done - Sprint 10]

    Acceptance Criteria: Run `tools/compare_lcd.py` and `tools/audit_sprites.py`; all gameplay sprites used in demo.model have Δ ≤ 1px vs mesh-centroid formula OR documented intentional override in `lcd_tune.txt`. No demo.model name fails `host_sprite_rect_ok`.

    Work Summary: Added `tools/audit_lcd_positions.py` — audits 45 demo.model sprites; skips `gen_lcd_positions.py` MANUAL_LCD overrides (marge1, maggie0–3) and `assets/lcd_tune.txt`. Passes with TASK-61 union-bbox rects.

    Feedback/Notes:

---

[TASK-46] Sprite coverage — all 96 materials blittable

    Type: Bug Fix

    Status: [Done - Sprint 10]

    Acceptance Criteria: Every material in `sprites.json` maps to valid rect + mask in generated headers (or explicit exclude list with reason, e.g. score-digit-only materials). `test/export_sprites.py` contact sheet shows 96 entries.

    Work Summary: Added `tools/audit_sprite_coverage.py`. 93 generated sprites cover all materials with visible atlas pixels. Excluded: `sprites` (atlas), `TEMP4` (unused), `digit02`/`digit12` (0 visible pixels — unused 7-segment middle segments).

    Feedback/Notes:

---

[TASK-50] Marge1 hair LCD clamp verification

    Type: Bug Fix

    Status: [Done - Sprint 10]

    Acceptance Criteria: `cupcake_sprite_lcd_resolve_y` keeps Marge hair inside 800px LCD in all gameplay frames. Validate with `--pin marge1` and demo frames that include `marge1`.

    Work Summary: `make test-marge-lcd` — header `lcd_y=115` + 390px height fits LCD; clamp min_y verified for extreme negatives after TASK-61 union bbox (hair_rows=31).

    Feedback/Notes:

---

## Sprint 11 — Audio

Ship SFX assets; SDL and retro-go playback; stop/fade on mode transitions.

| Task | Title | Status |
|------|-------|--------|
| TASK-38 | Extract and ship game audio assets | [Done - Sprint 11] |
| TASK-39 | SDL host audio playback for `CUPCAKE_CB_SFX` | [Done - Sprint 11] |
| TASK-40 | retro-go host audio playback | [Done - Sprint 11] |
| TASK-41 | Stop / fade sounds on mode transitions | [Done - Sprint 11] |

---

## Sprint 11

[TASK-38] Extract and ship game audio assets

    Type: Feature

    Status: [Done - Sprint 11]

    Acceptance Criteria: All 17 files from `game.model` present under `assets/audio/`: bonus, couch, cupcake, deliver, five, marge, miss, move, over, pacifier1, pacifier2, phase, points, start, step, throw, whoa (mp3/wav as original). Document extraction path from HAR if not in repo.

    Work Summary: `tools/extract_audio.py` pulls clips from the itch HAR (or `ignore/har_extracted/`); `make extract-audio`. Optional ffmpeg pass builds `{id}.wav` for SDL. Documented in `docs/AUDIO.md` and `docs/HAR.md`.

    Feedback/Notes: 

---

[TASK-39] SDL host audio playback for `CUPCAKE_CB_SFX`

    Type: Feature

    Status: [Done - Sprint 11]

    Acceptance Criteria: PC host loads and plays SFX by id; respects volume hints from `game.model` where practical; does not block game loop. Multiple concurrent one-shots allowed (points ticks).

    Work Summary: `platform/host_audio.c` (SDL_mixer): loads `assets/audio/{id}.wav`, volumes from game.model, 16 channels, `stop` → `Mix_HaltChannel(-1)`. Wired in `platform/sdl/main.c`. Requires `SDL2_mixer` + `make extract-audio`.

    Feedback/Notes: 

---

[TASK-40] retro-go host audio playback

    Type: Feature

    Status: [Done - Sprint 11]

    Acceptance Criteria: `platform/retrogo/main.c` plays SFX via odroid audio API (mirror Celeste port pattern). Assets loaded from device `CUPCAKE_ASSETS` path.

    Work Summary: `platform/retrogo/main.c` wires `CUPCAKE_CB_SFX` → `host_audio_play()`. LINUX_EMU build uses SDL_mixer (same as PC). Device firmware path: compile `host_audio.c` with `-DCUPCAKE_AUDIO_ODROID` (PCM voice mixer + `odroid_audio_submit`, `host_audio_pump` per frame). `Makefile.cupcake` links `host_audio` + SDL2_mixer.

    Feedback/Notes: 

---

[TASK-41] Stop / fade sounds on mode transitions

    Type: Feature

    Status: [Done - Sprint 11]

    Acceptance Criteria: `stop()` and phase/miss handlers call equivalent of JS `sounds.stop()` before new music/SFX. No overlapping `phase`/`over`/`start` clips after fast input.

    Work Summary: `host_sfx("stop")` → `Mix_HaltChannel(-1)` / voice pool clear. Called from `cupcake_stop()`, game over, phase complete/restart, demo/select, quick start, and `cupcake_on_start()` before intro SFX.

    Feedback/Notes: 

---

## Sprint 12 — Platform, QA & ship

Save-state hardening, retro-go bezel, automated tests, cheats, browser parity sign-off.

| Task | Title | Status |
|------|-------|--------|
| TASK-42 | Save-state version covers full simulation | [Done - Sprint 12] |
| TASK-43 | retro-go bezel / `CUPCAKE_CB_FRAME` support | [Done - Sprint 12] |
| TASK-44 | Automated demo replay regression test | [Done - Sprint 12] |
| TASK-51 | Optional debug cheats (`initCheats`) | [Done - Sprint 12] |
| TASK-59 | Makefile `make test` target | [Done - Sprint 12] |
| TASK-60 | End-to-end browser parity checklist | [In Progress - Sprint 12] |

---

## Sprint 12

[TASK-42] Save-state version covers full simulation

    Type: Feature

    Status: [Done - Sprint 12]

    Acceptance Criteria: Save/load includes entity grids, timer deadlines, scoreboard animation state, miss count, couch/marge/pacifier counters, and mode — not only current minimal `cupcake_state_t`. Loading mid-miss-sequence resumes correctly. Bump save format version with migration or invalidation.

    Work Summary: `cupcake_state_t` already exports full play state + timer snapshots (`cupcake_timers_export/import`) + RNG. Added `test/cupcake_save_game_test.c` (`make test-save-game`): play-field roundtrip via `cupcake_save_state`/`cupcake_load_state`, mid-`onM_Cupcake` miss timer restore after load. Save format remains v1 (`CUPCAKE_SAVE_MAGIC` / `CUPCAKE_SAVE_VERSION`).

    Feedback/Notes:

---

[TASK-43] retro-go bezel / `CUPCAKE_CB_FRAME` support

    Type: Feature

    Status: [Done - Sprint 12]

    Acceptance Criteria: retro-go host draws `screen.jpg` bezel like SDL PC build (or documents intentional LCD-only fullscreen). `CUPCAKE_CB_FRAME` implemented on both hosts consistently.

    Work Summary: Shared `host_bezel_blit_rgb565()` / `host_lcd_blit_rgb565()` / `host_lcd_rect_for_framebuffer()` in `platform/host_draw.c`. retro-go loads `screen.jpg`, `CUPCAKE_CB_FRAME` blits bezel + clears transparent LCD; sprites composite over bezel at the same rect math as SDL. SDL `CUPCAKE_CB_FRAME` clears LCD buffer; bezel still drawn in present before `cupcake_draw()`. `make test-host-bezel`. Device SD needs `screen.jpg` alongside atlas.

    Feedback/Notes:

---

[TASK-44] Automated demo replay regression test

    Type: Feature

    Status: [Done - Sprint 12]

    Acceptance Criteria: Test harness steps all 163 `cupcake_demo_frames` through renderer; optional PNG hash or pixel diff vs `test/export_sprites.py` reference / browser capture. Fails CI on sprite/LCD regression.

    Work Summary: `test/cupcake_demo_replay_test.c` (`make test-demo-replay`): asserts 163 frames, every demo sprite name resolves in `cupcake_sprites.h`, full 163×8-tick update/draw cycle wraps `demo_frame` to 0 with zero missing blits.

    Feedback/Notes:

---

[TASK-51] Optional debug cheats (`initCheats`)

    Type: Feature

    Status: [Done - Sprint 12]

    Acceptance Criteria: Alt+1..6 in play/start jumps to phase (debug threshold 1000 option optional). Guarded by compile flag `CUPCAKE_DEBUG_CHEATS`; no effect in release retro-go build.

    Work Summary: PC `Makefile` defines `CUPCAKE_DEBUG_CHEATS`; SDL `Alt+1..6` calls `cupcake_debug_cheat_phase()` (JS `initCheats` parity: sets phase/points, `onPhaseComplete`). Also `--start LEVEL,PHASE,SCORE` / `CUPCAKE_DEBUG_START` for score/phase test entry (`make test-debug-start`). retro-go `Makefile.cupcake` does not define the flag.

    Feedback/Notes:

---

[TASK-59] Makefile `make test` target

    Type: Feature

    Status: [Done - Sprint 12]

    Acceptance Criteria: Single command runs sprite export check + demo frame count validation + optional headless SDL smoke (init, 10 frames, exit 0). Document in README.

    Work Summary: `make test` / `make test-all` runs 40+ unit targets including new `test-smoke`, `test-demo-replay`, `test-save-game`, `test-debug-start`, and previously missing `test-phase-complete`. Smoke test: init + 10 update/draw cycles without SDL window.

    Feedback/Notes: Sprite PNG export remains manual (`python test/export_sprites.py`); see `test/README.md`.

---

[TASK-60] End-to-end browser parity checklist

    Type: Feature

    Status: [In Progress - Sprint 12]

    Acceptance Criteria: Manual test script covering: full demo loop, level 1 play to first miss, couch sit bonus, Marge five-cupcake delivery, pacifier catch, game over → CON → restart, level 2 faster timing. Sign-off recorded in this file's Feedback/Notes when complete.

    Work Summary: Added `docs/PARITY_CHECKLIST.md` with itch.io comparison checklist and sign-off table. Manual playthrough pending.

    Feedback/Notes:

---

## Sprint 13 — Bug fixes & parity polish

Post-ship parity fixes found during manual playtesting.

| Task | Title | Status |
|------|-------|--------|
| TASK-62 | Phase 2 must not start until phase music ends | [Done - Sprint 13] |

---

## Sprint 13

[TASK-62] Phase 2 must not start until phase music ends

    Type: Bug

    Status: [Done - Sprint 13]

    Acceptance Criteria: After reaching the phase score threshold, gameplay pauses for the ~4s phase SFX. Maggie and other entities must not advance during that music. Phase increments and entity reset happen only after the timer ends, then a 0.75s interstitial, then play resumes — matching itch.io.

    Work Summary: Split phase-complete end from immediate `onPhaseRestart`: `tmr_phase_complete_end` schedules 0.75s interstitial before `onPhaseStart` + resume. `onPhaseStart` leaves `enabled=0` until `cupcake_resume()` (start intro and debug start call resume; phase-complete path resumes in `sched_phase_restart_finish`). Removed extra `maggie_step` from `onPhaseStart`. `tmr_game_ot` guards on `cupcake_phase_celebration_active()`. Phase restart matches JS: `onPhaseStart` + `pause` immediately when music ends, then 0.75s before `resume` (Bart cannot move during interstitial). `make test-phase-complete` includes `test_no_game_ticks_during_phase_music`.

    Feedback/Notes: Manual verify: `run-pc.bat --start 1,1,9900`, score to 10k, confirm Maggie idle during phase music.

---

## Completed foundation (not ticketed)

These exist in `src/` / `platform/` / `tools/` and are prerequisites for the tasks above:

- Generated sprite rects, masks, LCD table, demo frame data (`make gen`)
- SDL PC host: atlas, bezel, LCD blit, debug pin / `lcd_tune.txt`, save slots F3/F4
- retro-go host skeleton: blit, save/load, button callback
- Demo replay at ~8 ticks/frame (0.25s model rate)
- Bart lane move prototype and position-based multi-sprite draw helper
- Cupcake stack draw helper for held cakes on lane
