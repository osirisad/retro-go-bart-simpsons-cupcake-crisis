#ifndef CUPCAKE_TIMER_H_
#define CUPCAKE_TIMER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Matches JS $G.Timer.speed (1.0 = normal). */
#ifndef CUPCAKE_TIMER_SPEED
#define CUPCAKE_TIMER_SPEED 1.0f
#endif

typedef void (*cupcake_timer_cb)(void *ctx, int tick);

/* Dynamic rate in seconds; `tick` is the value after the previous fire's increment. */
typedef float (*cupcake_timer_rate_fn)(void *ctx, int tick);

typedef enum {
    CUPCAKE_TMR_START,
    CUPCAKE_TMR_GAME,
    CUPCAKE_TMR_PHASE,
    CUPCAKE_TMR_MISS,
    CUPCAKE_TMR_ACTION,
    CUPCAKE_TMR_BONUS,
    CUPCAKE_TMR_COUCH,
    CUPCAKE_TMR_PACIFIER,
    CUPCAKE_TMR_DEMO,
    CUPCAKE_TMR_NAMED_COUNT,
} cupcake_timer_slot_t;

#define CUPCAKE_TMR_SCHEDULE_MAX 8

typedef struct cupcake_timer_config {
    float rate_sec;
    cupcake_timer_rate_fn rate_fn;
    void *rate_ctx;
    float delay_sec;
    int start_tick;
    int max_ticks; /* 0 = infinite (demo / game loop) */
    cupcake_timer_cb on_start;
    cupcake_timer_cb on_tick; /* JS OT */
    cupcake_timer_cb on_end;
    void *user;
    int persist;
} cupcake_timer_config_t;

typedef struct cupcake_timer {
    cupcake_timer_config_t cfg;
    float rate_override;
    float delay_remaining;
    float elapsed;
    int tick;
    uint8_t active;
    uint8_t paused;
    uint8_t skip_dt_once; /* set when started mid-frame — skip one update pass */
} cupcake_timer_t;

typedef struct cupcake_timers {
    cupcake_timer_t named[CUPCAKE_TMR_NAMED_COUNT];
    cupcake_timer_t schedule[CUPCAKE_TMR_SCHEDULE_MAX];
    uint8_t group_paused;
} cupcake_timers_t;

void cupcake_timers_init(cupcake_timers_t *tm);

/* Advance all active timers by dt_sec (call once per cupcake_update). */
void cupcake_timers_update(cupcake_timers_t *tm, float dt_sec);

/* True while a timer callback (on_start/on_tick/on_end) is running. */
int cupcake_timers_in_callback(void);

/* Optional hook after each top-level timers_update (deferred game work). */
void cupcake_timers_set_post_update(void (*fn)(void));

/*
 * Start (or restart) a named timer — mirrors new $G.Timer(opts).start().
 * Optional rate_override > 0 replaces cfg.rate_sec for this run (JS start(rate)).
 */
void cupcake_timer_start(cupcake_timers_t *tm, cupcake_timer_slot_t slot,
                         const cupcake_timer_config_t *cfg);
void cupcake_timer_start_rate(cupcake_timers_t *tm, cupcake_timer_slot_t slot,
                              const cupcake_timer_config_t *cfg, float rate_override);

void cupcake_timer_pause(cupcake_timers_t *tm, cupcake_timer_slot_t slot);
void cupcake_timer_resume(cupcake_timers_t *tm, cupcake_timer_slot_t slot);
void cupcake_timer_stop(cupcake_timers_t *tm, cupcake_timer_slot_t slot);

/* JS timers.pause() / resume() / stop() on the group (non-persist only). */
void cupcake_timers_pause(cupcake_timers_t *tm);
void cupcake_timers_resume(cupcake_timers_t *tm);
void cupcake_timers_stop(cupcake_timers_t *tm);

/*
 * JS timers.schedule(delay, fn) — one-shot after delay_sec.
 * Returns schedule slot index (0..CUPCAKE_TMR_SCHEDULE_MAX-1) or -1 if pool full.
 */
int cupcake_timers_schedule(cupcake_timers_t *tm, float delay_sec, cupcake_timer_cb fn,
                            void *ctx);

int cupcake_timer_active(const cupcake_timers_t *tm, cupcake_timer_slot_t slot);
int cupcake_timer_tick(const cupcake_timers_t *tm, cupcake_timer_slot_t slot);
int cupcake_timer_is_paused(const cupcake_timers_t *tm, cupcake_timer_slot_t slot);

#ifdef __cplusplus
}
#endif

#endif
