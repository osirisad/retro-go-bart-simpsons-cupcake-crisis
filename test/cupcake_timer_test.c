/*
 * Standalone checks for cupcake_timer.c — run: make test-timer
 */
#include "cupcake_timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail;
static int g_on_start;
static int g_on_tick;
static int g_on_end;
static int g_scheduled;
static int g_dynamic_rate_calls;
static float g_last_dynamic_rate;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void cb_start(void *ctx, int tick)
{
    (void)ctx;
    g_on_start = tick;
}

static void cb_tick(void *ctx, int tick)
{
    (void)ctx;
    g_on_tick = tick;
}

static void cb_end(void *ctx, int tick)
{
    (void)ctx;
    g_on_end = tick;
}

static void cb_schedule(void *ctx, int tick)
{
    (void)ctx;
    if (tick == 0)
        g_scheduled = 1;
}

static float rate_game(void *ctx, int tick)
{
    (void)ctx;
    g_dynamic_rate_calls++;
    g_last_dynamic_rate = (tick <= 1) ? 26.f / 30.f : 23.f / 30.f;
    return g_last_dynamic_rate;
}

static void test_start_sequence(void)
{
    cupcake_timers_t tm;
    cupcake_timer_config_t cfg;

    /* JS onStart: rate 3.16, ticks 2 */
    g_on_start = g_on_tick = g_on_end = -1;
    cupcake_timers_init(&tm);
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 3.16f;
    cfg.max_ticks = 2;
    cfg.on_start = cb_start;
    cfg.on_tick = cb_tick;
    cfg.on_end = cb_end;
    cupcake_timer_start(&tm, CUPCAKE_TMR_START, &cfg);

    if (g_on_start != 0)
        fail("start: onStart(0) on first fire");
    if (g_on_tick != -1)
        fail("start: OT must not run on tick 0");
    if (!cupcake_timer_active(&tm, CUPCAKE_TMR_START))
        fail("start: timer should stay active after first fire");

    cupcake_timers_update(&tm, 3.16f);
    if (g_on_tick != 1)
        fail("start: OT(1) after one interval");
    if (g_on_end != -1)
        fail("start: onEnd must not run before final tick");

    cupcake_timers_update(&tm, 3.16f);
    if (g_on_tick != 2)
        fail("start: OT(2) on second interval");
    if (g_on_end != 2)
        fail("start: onEnd(2)");
    if (cupcake_timer_active(&tm, CUPCAKE_TMR_START))
        fail("start: timer should stop after max_ticks");
}

static void test_schedule(void)
{
    cupcake_timers_t tm;

    g_scheduled = 0;
    cupcake_timers_init(&tm);
    if (cupcake_timers_schedule(&tm, 0.75f, cb_schedule, NULL) < 0)
        fail("schedule: pool slot");
    cupcake_timers_update(&tm, 0.74f);
    if (g_scheduled)
        fail("schedule: callback too early");
    cupcake_timers_update(&tm, 0.02f);
    if (!g_scheduled)
        fail("schedule: callback after delay");
}

static void test_pause_resume(void)
{
    cupcake_timers_t tm;
    cupcake_timer_config_t cfg;

    g_on_tick = 0;
    cupcake_timers_init(&tm);
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 1.f;
    cfg.max_ticks = 0;
    cfg.on_tick = cb_tick;
    cupcake_timer_start(&tm, CUPCAKE_TMR_GAME, &cfg);

    cupcake_timers_pause(&tm);
    cupcake_timers_update(&tm, 2.f);
    if (g_on_tick != 0)
        fail("pause: OT should not advance while group paused");

    cupcake_timers_resume(&tm);
    cupcake_timers_update(&tm, 1.f);
    if (g_on_tick != 1)
        fail("resume: OT should advance after resume");
}

static void test_dynamic_rate(void)
{
    cupcake_timers_t tm;
    cupcake_timer_config_t cfg;

    g_dynamic_rate_calls = 0;
    cupcake_timers_init(&tm);
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_fn = rate_game;
    cfg.max_ticks = 0;
    cfg.on_tick = cb_tick;
    cupcake_timer_start(&tm, CUPCAKE_TMR_GAME, &cfg);

    cupcake_timers_update(&tm, 26.f / 30.f);
    if (g_dynamic_rate_calls < 1)
        fail("dynamic rate: callback should be consulted");
    cupcake_timers_update(&tm, 23.f / 30.f);
    if (g_last_dynamic_rate < 0.7f)
        fail("dynamic rate: second interval should use phase-2 rate");
}

static void test_persist_on_group_stop(void)
{
    cupcake_timers_t tm;
    cupcake_timer_config_t cfg;

    cupcake_timers_init(&tm);
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 1.f;
    cfg.max_ticks = 0;
    cfg.persist = 1;
    cupcake_timer_start(&tm, CUPCAKE_TMR_DEMO, &cfg);
    cupcake_timer_start(&tm, CUPCAKE_TMR_GAME, &cfg);

    cupcake_timers_stop(&tm);
    if (!cupcake_timer_active(&tm, CUPCAKE_TMR_DEMO))
        fail("persist: demo timer survives group stop");
    if (cupcake_timer_active(&tm, CUPCAKE_TMR_GAME))
        fail("persist: non-persist timer should stop");
}

int main(void)
{
    test_start_sequence();
    test_schedule();
    test_pause_resume();
    test_dynamic_rate();
    test_persist_on_group_stop();

    if (g_fail)
        return 1;
    printf("cupcake_timer: all tests passed\n");
    return 0;
}
