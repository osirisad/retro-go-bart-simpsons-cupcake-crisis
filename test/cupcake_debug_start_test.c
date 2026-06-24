/*
 * Debug start (level / phase / score) — run: make test-debug-start
 */
#include "cupcake.h"

#include <stdio.h>

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void test_apply_enters_play(void)
{
    cupcake_play_state_t *p;

    cupcake_set_debug_start(2, 3, 15000u);
    cupcake_init();
    cupcake_apply_debug_start();
    p = cupcake_play_state();

    if (p->mode != CUPCAKE_MODE_PLAY)
        fail("debug start: play mode");
    if (!p->enabled)
        fail("debug start: enabled");
    if (p->level != 2)
        fail("debug start: level");
    if (p->phase != 3)
        fail("debug start: phase");
    if (p->points != 15000u)
        fail("debug start: points");
    if (p->scoreboard.phase != 3)
        fail("debug start: scoreboard phase");
    if (p->scoreboard.level != 0)
        fail("debug start: level overlay cleared");
}

static void test_near_threshold_triggers_phase_on_points(void)
{
    cupcake_timers_t *tm;

    cupcake_set_debug_start(1, 1, 9900u);
    cupcake_init();
    cupcake_apply_debug_start();
    tm = cupcake_timers();

    if (cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("debug start: phase timer not active yet");

    cupcake_add_points(100);
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("debug start: phase complete after crossing threshold");
}

static void test_at_threshold_triggers_immediately(void)
{
    cupcake_timers_t *tm;

    cupcake_set_debug_start(1, 1, 10000u);
    cupcake_init();
    cupcake_apply_debug_start();
    tm = cupcake_timers();

    if (!cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("debug start: phase timer when score already at threshold");
}

int main(void)
{
    test_apply_enters_play();
    test_near_threshold_triggers_phase_on_points();
    test_at_threshold_triggers_immediately();

    if (g_fail)
        return 1;
    printf("cupcake_debug_start: all tests passed\n");
    return 0;
}
