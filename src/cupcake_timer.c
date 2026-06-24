/*
 * Portable timer group — RetroFab $G.Timer / $G.Timers semantics.
 */
#include "cupcake_timer.h"

#include <string.h>

static int g_timer_update_depth;
static void (*g_timers_post_update)(void);

int cupcake_timers_in_callback(void)
{
    return g_timer_update_depth > 0 ? 1 : 0;
}

void cupcake_timers_set_post_update(void (*fn)(void))
{
    g_timers_post_update = fn;
}

static int timer_paused(const cupcake_timers_t *tm, const cupcake_timer_t *t)
{
    return (tm && tm->group_paused) || t->paused;
}

static float timer_rate_sec(const cupcake_timer_t *t)
{
    if (t->rate_override > 0.f)
        return t->rate_override * CUPCAKE_TIMER_SPEED;
    if (t->cfg.rate_fn)
        return t->cfg.rate_fn(t->cfg.rate_ctx, t->tick) * CUPCAKE_TIMER_SPEED;
    return t->cfg.rate_sec * CUPCAKE_TIMER_SPEED;
}

static void timer_stop_one(cupcake_timer_t *t)
{
    t->active = 0;
    t->paused = 0;
    t->elapsed = 0.f;
    t->delay_remaining = 0.f;
    t->rate_override = 0.f;
}

static void timer_fire(cupcake_timers_t *tm, cupcake_timer_t *t)
{
    if (!t->active)
        return;

    if (!timer_paused(tm, t)) {
        const int tick = t->tick;
        const int start = t->cfg.start_tick;

        if (tick == start && t->cfg.on_start)
            t->cfg.on_start(t->cfg.user, tick);
        if (tick > 0 && t->cfg.on_tick)
            t->cfg.on_tick(t->cfg.user, tick);
        if (t->cfg.max_ticks > 0 && tick >= t->cfg.max_ticks) {
            if (t->cfg.on_end)
                t->cfg.on_end(t->cfg.user, tick);
            timer_stop_one(t);
            return;
        }
        t->tick++;
    }
}

static void timer_begin(cupcake_timers_t *tm, cupcake_timer_t *t, const cupcake_timer_config_t *cfg,
                        float rate_override)
{
    memset(t, 0, sizeof *t);
    t->cfg = *cfg;
    t->tick = cfg->start_tick;
    t->active = 1;
    if (rate_override > 0.f)
        t->rate_override = rate_override;
    if (cfg->delay_sec > 0.f) {
        t->delay_remaining = cfg->delay_sec;
        if (g_timer_update_depth > 0)
            t->skip_dt_once = 1;
    } else {
        timer_fire(tm, t);
        if (g_timer_update_depth > 0)
            t->skip_dt_once = 1;
    }
}

static void timer_update_one(cupcake_timers_t *tm, cupcake_timer_t *t, float dt_sec)
{
    float rate;

    if (!t->active)
        return;

    if (t->skip_dt_once) {
        t->skip_dt_once = 0;
        return;
    }

    if (t->delay_remaining > 0.f) {
        t->delay_remaining -= dt_sec;
        if (t->delay_remaining > 0.f)
            return;
        timer_fire(tm, t);
        if (!t->active)
            return;
    }

    t->elapsed += dt_sec;
    rate = timer_rate_sec(t);
    if (rate <= 0.f)
        rate = 1e-6f;

    while (t->active && t->elapsed >= rate) {
        t->elapsed -= rate;
        timer_fire(tm, t);
        if (!t->active)
            break;
        rate = timer_rate_sec(t);
        if (rate <= 0.f)
            rate = 1e-6f;
    }
}

void cupcake_timers_init(cupcake_timers_t *tm)
{
    memset(tm, 0, sizeof *tm);
}

void cupcake_timers_update(cupcake_timers_t *tm, float dt_sec)
{
    int i;

    if (!tm || dt_sec <= 0.f)
        return;

    g_timer_update_depth++;
    for (i = 0; i < CUPCAKE_TMR_NAMED_COUNT; i++)
        timer_update_one(tm, &tm->named[i], dt_sec);
    for (i = 0; i < CUPCAKE_TMR_SCHEDULE_MAX; i++)
        timer_update_one(tm, &tm->schedule[i], dt_sec);
    g_timer_update_depth--;
    if (g_timer_update_depth == 0 && g_timers_post_update)
        g_timers_post_update();
}

void cupcake_timer_start(cupcake_timers_t *tm, cupcake_timer_slot_t slot,
                         const cupcake_timer_config_t *cfg)
{
    cupcake_timer_start_rate(tm, slot, cfg, 0.f);
}

void cupcake_timer_start_rate(cupcake_timers_t *tm, cupcake_timer_slot_t slot,
                              const cupcake_timer_config_t *cfg, float rate_override)
{
    if (!tm || !cfg || slot < 0 || slot >= CUPCAKE_TMR_NAMED_COUNT)
        return;
    timer_begin(tm, &tm->named[slot], cfg, rate_override);
}

void cupcake_timer_pause(cupcake_timers_t *tm, cupcake_timer_slot_t slot)
{
    if (!tm || slot < 0 || slot >= CUPCAKE_TMR_NAMED_COUNT)
        return;
    tm->named[slot].paused = 1;
}

void cupcake_timer_resume(cupcake_timers_t *tm, cupcake_timer_slot_t slot)
{
    if (!tm || slot < 0 || slot >= CUPCAKE_TMR_NAMED_COUNT)
        return;
    tm->named[slot].paused = 0;
}

void cupcake_timer_stop(cupcake_timers_t *tm, cupcake_timer_slot_t slot)
{
    if (!tm || slot < 0 || slot >= CUPCAKE_TMR_NAMED_COUNT)
        return;
    timer_stop_one(&tm->named[slot]);
}

void cupcake_timers_pause(cupcake_timers_t *tm)
{
    if (tm)
        tm->group_paused = 1;
}

void cupcake_timers_resume(cupcake_timers_t *tm)
{
    if (tm)
        tm->group_paused = 0;
}

void cupcake_timers_stop(cupcake_timers_t *tm)
{
    int i;

    if (!tm)
        return;

    for (i = 0; i < CUPCAKE_TMR_NAMED_COUNT; i++) {
        if (!tm->named[i].cfg.persist)
            timer_stop_one(&tm->named[i]);
    }
    for (i = 0; i < CUPCAKE_TMR_SCHEDULE_MAX; i++)
        timer_stop_one(&tm->schedule[i]);
}

int cupcake_timers_schedule(cupcake_timers_t *tm, float delay_sec, cupcake_timer_cb fn, void *ctx)
{
    cupcake_timer_config_t cfg;
    int i;

    if (!tm || !fn || delay_sec < 0.f)
        return -1;

    for (i = 0; i < CUPCAKE_TMR_SCHEDULE_MAX; i++) {
        if (!tm->schedule[i].active)
            break;
    }
    if (i >= CUPCAKE_TMR_SCHEDULE_MAX)
        return -1;

    memset(&cfg, 0, sizeof cfg);
    cfg.delay_sec = delay_sec;
    cfg.rate_sec = delay_sec; /* restore matching after save/load */
    cfg.max_ticks = 1;
    cfg.on_start = fn;
    cfg.user = ctx;
    timer_begin(tm, &tm->schedule[i], &cfg, 0.f);
    return i;
}

int cupcake_timer_active(const cupcake_timers_t *tm, cupcake_timer_slot_t slot)
{
    if (!tm || slot < 0 || slot >= CUPCAKE_TMR_NAMED_COUNT)
        return 0;
    return tm->named[slot].active ? 1 : 0;
}

int cupcake_timer_tick(const cupcake_timers_t *tm, cupcake_timer_slot_t slot)
{
    if (!tm || slot < 0 || slot >= CUPCAKE_TMR_NAMED_COUNT)
        return 0;
    return tm->named[slot].tick;
}

int cupcake_timer_is_paused(const cupcake_timers_t *tm, cupcake_timer_slot_t slot)
{
    if (!tm || slot < 0 || slot >= CUPCAKE_TMR_NAMED_COUNT)
        return 0;
    return tm->named[slot].paused ? 1 : 0;
}
