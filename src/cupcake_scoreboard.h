#ifndef CUPCAKE_SCOREBOARD_H

#define CUPCAKE_SCOREBOARD_H



#include "cupcake_state.h"
#include "cupcake_timer.h"



#include <stddef.h>

#include <stdint.h>



typedef void (*cupcake_scoreboard_draw_fn)(const char *sprite_name, void *ctx);

/* JS scoreboard: hi-score / run score digits vs L-N / P-N text — mutually exclusive. */
typedef enum {
    CUPCAKE_SB_DISP_VALUE = 0, /* demo hi-score (value field) */
    CUPCAKE_SB_DISP_SCORE = 1, /* run score (score field) */
    CUPCAKE_SB_DISP_LEVEL = 2, /* L-N overlay only */
    CUPCAKE_SB_DISP_PHASE = 3, /* P-N overlay only */
} cupcake_sb_disp_t;



/* JS scoreboard.setValue: rightmost slot = index 0, optional alwayson pad. */

int cupcake_scoreboard_slot_digit(uint32_t value, int alwayson, int num_digits,

                                  int slot_from_right);



void cupcake_scoreboard_digit_sprite(int column, int segment, char *buf, size_t bufsz);



void cupcake_scoreboard_draw_slot(int column, int digit, cupcake_scoreboard_draw_fn draw,

                                  void *ctx);



void cupcake_scoreboard_draw_segments(int column, const uint8_t *segments, unsigned count,

                                      cupcake_scoreboard_draw_fn draw, void *ctx);



/* 5-digit score field (columns 0..4). */

void cupcake_scoreboard_draw_digits(uint32_t value, int alwayson, int num_digits,

                                    cupcake_scoreboard_draw_fn draw, void *ctx);



/* JS scoreboard.text: up to 3 chars on columns 4, 3, 2 (P-1, L-2, CON, …). */

void cupcake_scoreboard_draw_text(const char *text, cupcake_scoreboard_draw_fn draw, void *ctx);



/* Mode-aware scoreboard: demo value / start-play-over score + phase / level / CON overlay. */

void cupcake_scoreboard_draw(const cupcake_play_state_t *play, cupcake_scoreboard_draw_fn draw,

                             void *ctx);

/* JS scoreboard.phase = clamp(phase, 1..6) — start / play / phase restart. */
void cupcake_scoreboard_set_phase(cupcake_play_state_t *play, int phase);

/* JS scoreboard.level — 0 hidden (attract), 1..2 shows L-N overlay. */
void cupcake_scoreboard_set_level(cupcake_play_state_t *play, int level);

/* Show hi-score digits (demo attract). */
void cupcake_scoreboard_show_value(cupcake_play_state_t *play, uint32_t value);

/* Show run score digits (play / start tick 1–2). */
void cupcake_scoreboard_show_run_score(cupcake_play_state_t *play);



/* Demo level pick: scoreboard.text = "L-{level}" on columns 4..2 (JS AcclaimSuperplayScoreboard). */

void cupcake_scoreboard_draw_demo_level(const cupcake_play_state_t *play,

                                        cupcake_scoreboard_draw_fn draw, void *ctx);



/* Demo attract: 5-digit hi-score (alwayson=2), no level indicator. */

void cupcake_scoreboard_draw_demo_hiscore(const cupcake_play_state_t *play,

                                          cupcake_scoreboard_draw_fn draw, void *ctx);

typedef void (*cupcake_scoreboard_sfx_fn)(const char *id);
typedef void (*cupcake_scoreboard_void_fn)(void);
typedef void (*cupcake_scoreboard_bonus_tick_fn)(void *ctx, int tick);
typedef void (*cupcake_scoreboard_bonus_end_fn)(void *ctx);

typedef struct {
    cupcake_scoreboard_void_fn pause;
    cupcake_scoreboard_void_fn resume;
    cupcake_scoreboard_sfx_fn sfx;
    cupcake_scoreboard_void_fn on_score_change;
} cupcake_scoreboard_host_t;

/* JS scoreboard.addBonus({rate, points, increment, OT, onEnd}). */
typedef struct {
    float rate_sec;
    int point_ticks;
    int increment;
    uint8_t play_points_sfx;
    uint8_t play_five_on_first;
    cupcake_scoreboard_bonus_tick_fn on_tick;
    cupcake_scoreboard_bonus_end_fn on_end;
    void *user_ctx;
} cupcake_scoreboard_bonus_cfg_t;

void cupcake_scoreboard_set_host(const cupcake_scoreboard_host_t *host);

int cupcake_scoreboard_add_bonus_ex(cupcake_play_state_t *play, cupcake_timers_t *timers,
                                    const cupcake_scoreboard_bonus_cfg_t *cfg);

void cupcake_scoreboard_timers_restore(cupcake_play_state_t *play, cupcake_timers_t *timers);

#endif

