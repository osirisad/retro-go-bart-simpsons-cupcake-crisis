#ifndef CUPCAKE_H_

#define CUPCAKE_H_



#include <stddef.h>

#include <stdint.h>



#ifdef __cplusplus

extern "C" {

#endif



typedef enum {

    CUPCAKE_BTN_LEFT = 1 << 0,

    CUPCAKE_BTN_RIGHT = 1 << 1,

    CUPCAKE_BTN_UP = 1 << 2,

    CUPCAKE_BTN_DOWN = 1 << 3,

    CUPCAKE_BTN_ACTION = 1 << 4,

    CUPCAKE_BTN_SELECT = 1 << 5,

    CUPCAKE_BTN_LEVEL1 = 1 << 6,

    CUPCAKE_BTN_LEVEL2 = 1 << 7,

    CUPCAKE_BTN_SOUND = 1 << 8,

} cupcake_btn_t;

typedef enum {
    CUPCAKE_MOVE_LEFT,
    CUPCAKE_MOVE_RIGHT,
    CUPCAKE_MOVE_UP,
    CUPCAKE_MOVE_DOWN,
} cupcake_move_t;



/* Pass as lcd_x/lcd_y to CUPCAKE_CB_SPR to use mesh position from cupcake_sprite_lcd.h */

#define CUPCAKE_LCD_AUTO (-1)



typedef enum {

    CUPCAKE_CB_FRAME,   /* host draws screen.jpg bezel */

    CUPCAKE_CB_SPR,     /* (const char *name, int lcd_x, int lcd_y) */

    CUPCAKE_CB_BTN,     /* (int button_index) -> held */

    CUPCAKE_CB_SFX,     /* (const char *id) */

    CUPCAKE_CB_SOUND_TOGGLE, /* JS onRelease Sound — host toggles mute */

} cupcake_cb_type_t;



/*

 * Host callback (non-variadic — safe across translation units):

 *   CUPCAKE_CB_FRAME  str=NULL

 *   CUPCAKE_CB_SPR    str=name, int0=lcd_x, int1=lcd_y

 *   CUPCAKE_CB_BTN    str=NULL, int0=button_index

 *   CUPCAKE_CB_SFX    str=sfx_id

 *   CUPCAKE_CB_SOUND_TOGGLE  str=NULL (on button release)

 */

typedef int (*cupcake_cb_t)(cupcake_cb_type_t type, const char *str_arg, int int_arg0,

                            int int_arg1);



#include "cupcake_state.h"

#include "cupcake_timer.h"

#include "cupcake_rng.h"



void cupcake_set_callback(cupcake_cb_t cb);

void cupcake_init(void);

void cupcake_update(void);

void cupcake_draw(void);



void cupcake_set_buttons(uint16_t buttons);

/* Set current/previous buttons equal — no press/release edges (after retro-go menu). */
void cupcake_buttons_sync(uint16_t buttons);



/* Always draw `name` for LCD alignment work; NULL clears. freeze_demo=1 pauses demo frames. */

void cupcake_set_debug_pin(const char *name, int freeze_demo);

/* Pin every sprite listed in assets/lcd_tune.txt; returns count (0 if file empty). */

int cupcake_set_debug_pin_from_tune(int freeze_demo);

/* If 1, demo/play draws only pinned sprite(s) (empty LCD + pin). */

void cupcake_set_debug_pin_solo(int solo);



/* Log header + lcd_tune.txt position for one sprite, or all tune entries if name is NULL. */

void cupcake_log_sprite_lcd(const char *name);



size_t cupcake_state_size(void);

void cupcake_save_state(void *buf);

int cupcake_load_state(const void *buf);



const cupcake_state_t *cupcake_get_state(void);

cupcake_play_state_t *cupcake_play_state(void);



/* Game timer group (RetroFab $G.Timers). Valid after cupcake_init(). */

cupcake_timers_t *cupcake_timers(void);

/*
 * JS AcclaimCupcakeCrisis.onStart(phase, show_level):
 * demo / Quick Start / CON continue → start mode (3.16s, 2 ticks) → play.
 * show_level: scoreboard shows level digit instead of phase (Quick Start).
 */
void cupcake_on_start(int phase, int show_level);

/* JS onGameOver / onM_ when miss.count == 3. */
void cupcake_on_game_over(void);

/* JS miss.increase() + branch to game over or phase restart (restart in TASK-14). */
void cupcake_miss_increase(void);

/* JS pause / resume — pauses `game` timer slot + clears `enabled`. */
void cupcake_pause(void);
void cupcake_resume(void);
int cupcake_is_paused(void); /* play mode with enabled false */

/* Sequence entry points (pause/resume wired; entity detail in later tasks). */
void cupcake_on_m_cupcake(int lane);
void cupcake_on_m_couch(void);
void cupcake_on_phase_complete(void);
void cupcake_on_phase_restart(void);
void cupcake_bart_sit_bonus(int couch_bonus_points);
void cupcake_scoreboard_add_bonus(float rate_sec, int point_ticks, int increment);
void cupcake_add_points(int points);

/* JS setThreshold(b) — default 10000, debug 1000. */
void cupcake_set_threshold(uint32_t threshold);

/* JS this.record — off by default; fixes couch/pacifier spawn intervals for capture. */
void cupcake_set_record(int on);
int cupcake_record_mode(void);

/* Run score needed to complete current phase: phase * phase_threshold. */
uint32_t cupcake_phase_score_target(const cupcake_play_state_t *play);

/* JS timers['game'] rate (sec/tick) from level/phase tables + 90% threshold slowdown. */
float cupcake_game_tick_rate_sec(const cupcake_play_state_t *play);

/* Per-level hi-score file. Call cupcake_hiscore_set_path before cupcake_init() on hosts. */
void cupcake_hiscore_default_path(char *buf, size_t bufsz);
void cupcake_hiscore_set_path(const char *path);

/* JS onPress handlers (TASK-07). */
void cupcake_on_move(cupcake_move_t dir);

/* JS Bart.catchCupcake / catchPacifier (TASK-20). */
int cupcake_bart_catch_cupcake(cupcake_play_state_t *p, int lane);
void cupcake_bart_catch_pacifier(cupcake_play_state_t *p);
/* Aircakes landing on lane — catch/miss or grid (TASK-24 calls). */
void cupcake_aircakes_land_at_lane(cupcake_play_state_t *p, int lane);
/* Pacifier loop-3 auto-catch (TASK-32 calls from step). */
void cupcake_pacifier_on_loop3(cupcake_play_state_t *p);

/* JS onQuickStart(Level1|Level2): stop sim, set level, onStart(1, true). */
void cupcake_on_quick_start(int level);

/*
 * PC/dev: skip attract + start intro and enter play at level/phase/points.
 * Call cupcake_set_debug_start() before cupcake_init(), then cupcake_apply_debug_start()
 * after init (and audio). Points is run score (not hi-score). If points already meets the
 * current phase threshold, phase-complete runs immediately.
 */
void cupcake_set_debug_start(int level, int phase, uint32_t points);
void cupcake_apply_debug_start(void);
int cupcake_debug_start_pending(void);

/* JS initCheats Alt+1..6 — PC dev when CUPCAKE_DEBUG_CHEATS is defined. */
void cupcake_debug_cheat_phase(int phase_1_to_6);



#ifdef __cplusplus

}

#endif



#endif

