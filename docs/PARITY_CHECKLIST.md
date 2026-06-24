# Browser parity checklist (TASK-60)

Manual sign-off against [itch.io](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis) or a local HAR build. Compare side-by-side with the PC port (`run-pc.bat`).

## Attract / demo

- [ ] Demo loops continuously with hi-score on scoreboard
- [ ] Select cycles level 0 → L-1 → L-2 → attract (demo freezes on L-1/L-2)
- [ ] Action with level selected runs start intro then play

## Level 1 play

- [ ] Quick start key `1` or Level1 → L-1 intro → phase 1 play
- [ ] Arrow keys move Bart when enabled
- [ ] Catch floor cupcake → stack grows, points increase
- [ ] Catch flying cupcake / pacifier paths match browser
- [ ] First miss: Bart miss pose, aircake flash, miss icon, game pauses then resumes
- [ ] Third miss: game over sound, play stops
- [ ] Select after over → scoreboard shows `CON`; Action continues at same phase

## Phases

- [ ] At 10,000 run points: phase music (~4s), scoreboard shows next phase (P-2…)
- [ ] Play auto-resumes after phase music (no button press)
- [ ] Speed increases each phase within same difficulty level
- [ ] Phase 6 still playable (no crash / soft-lock)

## Couch & Marge

- [ ] Couch spawns and animates across bottom
- [ ] Bart Up/Down at couch → sit bonus (bonus SFX, animated score)
- [ ] Bart Action at position 1 throws toward Marge
- [ ] Five cupcakes delivered → Marge collect bonus

## Level 2

- [ ] Quick start `2` uses faster tick table than level 1 at same phase
- [ ] Hi-score tracked separately per level

## Audio (PC)

- [ ] Start, move, catch, miss, phase, bonus, game over SFX audible
- [ ] SFX stop on mode transitions (no overlap into intro)

## PC debug aids

- [ ] `--start 1,1,9900` jumps near phase-1 threshold
- [ ] Alt+1…6 in play jumps to phase (debug build only)

## Sign-off

| Date | Tester | Build | Notes |
|------|--------|-------|-------|
|      |        |       |       |

Record results in `SPRINT_BOARD.md` TASK-60 Feedback/Notes when complete.
