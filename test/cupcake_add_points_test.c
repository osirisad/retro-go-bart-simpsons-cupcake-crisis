/*
 * TASK-13 addPoints and phase threshold — run: make test-add-points
 */
#include "cupcake.h"

#include <stdio.h>

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void setup_play(void)
{
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_start(1, 0);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_PLAY)
        fail("setup: expected play mode");
}

static void test_add_points_updates_scoreboard(void)
{
    cupcake_play_state_t *p;

    setup_play();
    p = cupcake_play_state();
    p->points = 0;
    p->scoreboard.score = 0;
    cupcake_set_threshold(10000);

    cupcake_add_points(100);
    if (p->points != 100u || p->scoreboard.score != 100u)
        fail("addPoints: updates run score and scoreboard");

    cupcake_add_points(0);
    if (p->points != 100u)
        fail("addPoints: ignores zero");
}

static void test_default_threshold_triggers_phase_complete(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    setup_play();
    p = cupcake_play_state();
    tm = cupcake_timers();
    p->phase = 1;
    cupcake_set_threshold(10000);

    cupcake_add_points(9999);
    if (cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("threshold: no phase timer below target");

    cupcake_add_points(1);
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("threshold: phase complete at phase*10000");
}

static void test_set_threshold_debug_value(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    setup_play();
    p = cupcake_play_state();
    tm = cupcake_timers();
    p->phase = 1;
    cupcake_set_threshold(1000);

    if (cupcake_phase_score_target(p) != 1000u)
        fail("setThreshold: target = phase * threshold");

    cupcake_add_points(1000);
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("setThreshold(1000): phase complete at 1000");
}

static void test_phase_two_needs_double_threshold(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    setup_play();
    p = cupcake_play_state();
    tm = cupcake_timers();
    p->phase = 2;
    cupcake_set_threshold(10000);

    cupcake_add_points(19999);
    if (cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("phase 2: no complete below 20000");

    cupcake_add_points(1);
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("phase 2: complete at 20000");
}

static void test_init_default_threshold(void)
{
    cupcake_init();
    if (cupcake_play_state()->phase_threshold != 10000u)
        fail("init: default threshold 10000");
}

int main(void)
{
    test_add_points_updates_scoreboard();
    test_default_threshold_triggers_phase_complete();
    test_set_threshold_debug_value();
    test_phase_two_needs_double_threshold();
    test_init_default_threshold();

    if (g_fail)
        return 1;
    printf("cupcake_add_points: all tests passed\n");
    return 0;
}
