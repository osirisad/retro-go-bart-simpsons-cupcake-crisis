#ifndef CUPCAKE_STATE_H_
#define CUPCAKE_STATE_H_

#include <stddef.h>
#include <stdint.h>

#include "cupcake_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CUPCAKE_SAVE_MAGIC   0x43555033u /* "CUP3" */
#define CUPCAKE_SAVE_VERSION 1u

#define CUPCAKE_GRID_LANES     5
#define CUPCAKE_GRID_SLOTS     5
#define CUPCAKE_AIRCAKE_COUNT  10
#define CUPCAKE_MISS_MAX       3
#define CUPCAKE_PHASE_MAX      6
#define CUPCAKE_LEVEL_MAX      2

typedef enum {
    CUPCAKE_MODE_DEMO,
    CUPCAKE_MODE_START,
    CUPCAKE_MODE_PLAY,
    CUPCAKE_MODE_OVER,
} cupcake_mode_t;

typedef struct {
    int8_t pos;        /* 0-5 ($P.Bart position) */
    uint8_t count;     /* cupcakes held, max 5 */
    int8_t miss_index; /* -1 = normal pose; 6 / 9 = miss animation (bart6/bart9) */
    uint16_t visible;  /* bit i = bart{i} material visible (bart0..bart9) */
} cupcake_bart_state_t;

typedef struct {
    /* Bit ((lane - 1) * 5 + (slot - 1)), lane 1..5 slot 1..5 (row 5 = cake41..45). */
    uint32_t visible;
    uint8_t group_visible;
} cupcake_grid_state_t;

typedef struct {
    /* Bit i = flying sprite cake{i} visible (cake0..cake9). */
    uint16_t visible;
    uint8_t group_visible;
} cupcake_aircakes_state_t;

typedef struct {
    uint8_t loop;    /* 0..3 throw cycle */
    uint8_t index;   /* maggie0..3 material index */
    uint8_t visible;
} cupcake_maggie_state_t;

typedef struct {
    uint8_t visible; /* marge1 on LCD */
    uint8_t loop;    /* on-screen hide step counter */
} cupcake_marge_state_t;

typedef struct {
    uint8_t onscreen;
    uint8_t anim; /* 0..4 during couch0..couch4 spawn sequence */
    uint8_t frame_visible; /* bit i = couch{i} material visible (TASK-18/30) */
    uint16_t counter;
    uint16_t next; /* game ticks until couch.start(true) */
} cupcake_couch_state_t;

typedef struct {
    uint8_t visible;
    uint8_t index; /* 1 = pacifier1, 2 = pacifier2 */
    uint8_t loop;
    uint16_t counter;
    uint16_t next;
} cupcake_pacifier_state_t;

typedef struct {
    uint8_t count; /* 0..3 miss icons (miss1..miss3) */
} cupcake_miss_state_t;

typedef struct {
    uint32_t score;
    uint32_t value;
    uint32_t hi_score[3]; /* index by level 0..2 */
    uint8_t phase;         /* 1..6 display */
    uint8_t level;         /* 0 demo, 1..2 play */
    uint8_t show_con;      /* scoreboard.text == "CON" */
    uint8_t disp;          /* cupcake_sb_disp_t — digits vs L/P overlay */
    uint8_t bonus_active;
    float bonus_rate;
    uint16_t bonus_points_left;
    uint16_t bonus_increment;
} cupcake_scoreboard_state_t;

/* Numeric timer runtime — callbacks re-bound after load (see cupcake_timer_restore). */
typedef struct {
    uint8_t active;
    uint8_t paused;
    uint8_t persist;
    float elapsed;
    float delay_remaining;
    float rate_override;
    float rate_sec;
    int tick;
    int start_tick;
    int max_ticks;
} cupcake_timer_saved_t;

typedef struct {
    cupcake_mode_t mode;
    uint8_t enabled;
    uint8_t level;
    uint8_t phase;
    uint32_t points;
    uint32_t phase_threshold;
    uint32_t game_tick; /* $P.Marge/Couch step tick argument */
    uint8_t record;     /* JS this.record — fixed couch/pacifier spawn intervals */
    cupcake_bart_state_t bart;
    cupcake_grid_state_t grid;
    cupcake_aircakes_state_t aircakes;
    cupcake_maggie_state_t maggie;
    cupcake_marge_state_t marge;
    cupcake_couch_state_t couch;
    cupcake_pacifier_state_t pacifier;
    cupcake_miss_state_t miss;
    cupcake_scoreboard_state_t scoreboard;
} cupcake_play_state_t;

typedef struct {
    cupcake_play_state_t play;
    uint32_t frame;
    uint16_t demo_frame;
    uint8_t demo_tick;
    uint32_t rng_state;
    cupcake_timer_saved_t timers_named[CUPCAKE_TMR_NAMED_COUNT];
    cupcake_timer_saved_t timers_sched[CUPCAKE_TMR_SCHEDULE_MAX];
    uint8_t timers_group_paused;
} cupcake_state_t;

/* --- grid helpers (lane/slot 1-based) --- */
void cupcake_grid_clear(cupcake_grid_state_t *g);
void cupcake_grid_set_visible(cupcake_grid_state_t *g, int lane, int slot, int on);
int cupcake_grid_is_visible(const cupcake_grid_state_t *g, int lane, int slot);
void cupcake_grid_sprite_name(int lane, int slot, char *buf, size_t bufsz);
typedef void (*cupcake_grid_draw_fn)(const char *name, void *ctx);
void cupcake_grid_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx);

void cupcake_aircakes_clear(cupcake_aircakes_state_t *a);
void cupcake_aircake_set_visible(cupcake_aircakes_state_t *a, int index, int on);
int cupcake_aircake_is_visible(const cupcake_aircakes_state_t *a, int index);
void cupcake_aircake_sprite_name(int index, char *buf, size_t bufsz);
void cupcake_aircakes_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx);

void cupcake_play_state_reset(cupcake_play_state_t *p);
void cupcake_play_state_start_demo(cupcake_play_state_t *p);
void cupcake_play_state_start_phase(cupcake_play_state_t *p, int bart_lane);

int cupcake_phase_clamp(int phase);

/* JS entity.start() resets — used by start / onPhaseStart (TASK-04+). */
void cupcake_bart_start(cupcake_play_state_t *p, int pos);
/* JS Bart.position setter — hide prior layers/stack, show materials for pos (TASK-16+). */
void cupcake_bart_set_position(cupcake_play_state_t *p, int pos);
int cupcake_bart_material_visible(const cupcake_play_state_t *p, int material_index);
int cupcake_bart_stack_lane(int pos);
/* Layer bitmask for position N (TASK-49: pos 0 = bart0+bart1 only, not lane-1 extras). */
uint16_t cupcake_bart_position_layers(int pos);
/* Miss pose sprites bart6 / bart9 (TASK-22). */
void cupcake_bart_set_miss_pose(cupcake_play_state_t *p, int index);
void cupcake_bart_clear_miss_pose(cupcake_play_state_t *p);
int cupcake_bart_miss_index(const cupcake_play_state_t *p);
/* JS marge.collect() — full delivery bonus in TASK-29; move side effects in TASK-17. */
int cupcake_marge_collect(cupcake_play_state_t *p);
void cupcake_maggie_start(cupcake_play_state_t *p);
/* JS miss.start(n) — optional initial count shows miss1..missn. */
void cupcake_miss_start(cupcake_play_state_t *p, int initial);
void cupcake_miss_decrease(cupcake_play_state_t *p);
void cupcake_cupcakes_start(cupcake_play_state_t *p);
void cupcake_aircakes_start(cupcake_play_state_t *p);
void cupcake_pacifier_start(cupcake_play_state_t *p);
void cupcake_marge_start(cupcake_play_state_t *p);
void cupcake_couch_start(cupcake_play_state_t *p);
/* Bonus tick count from visible couch frame at sit time (couch3→1, couch2→2, couch1→4). */
int cupcake_couch_sit_bonus_ticks(const cupcake_couch_state_t *c);
void cupcake_couch_set_frame_visible(cupcake_couch_state_t *c, int index, int on);

void cupcake_maggie_sprite_name(int index, char *buf, size_t bufsz);
void cupcake_maggie_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx);
void cupcake_couch_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx);
void cupcake_marge_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx);
void cupcake_pacifier_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx);
void cupcake_miss_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx);
void cupcake_bart_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx);
void cupcake_play_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx);

/* JS entity.step() — full logic in later entity tasks; wired from game timer OT. */
void cupcake_cupcakes_step(cupcake_play_state_t *p);
void cupcake_aircakes_step(cupcake_play_state_t *p);
void cupcake_maggie_step(cupcake_play_state_t *p);
void cupcake_marge_step(cupcake_play_state_t *p, int tick);
void cupcake_couch_step(cupcake_play_state_t *p, int tick);
void cupcake_pacifier_step(cupcake_play_state_t *p);

void cupcake_timer_export(const cupcake_timer_t *tm, cupcake_timer_saved_t *out);
void cupcake_timer_import(cupcake_timer_t *tm, const cupcake_timer_saved_t *in);

void cupcake_timers_export(const cupcake_timers_t *src, cupcake_state_t *dst);
void cupcake_timers_import(cupcake_timers_t *dst, const cupcake_state_t *src);

#ifdef __cplusplus
}
#endif

#endif
