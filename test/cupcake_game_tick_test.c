/*
 * TASK-15 game tick timer — run: make test-game-tick
 */
#include "cupcake.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static int near(float a, float b)
{
    return fabsf(a - b) < 1e-5f;
}

static void expect_rate(const cupcake_play_state_t *p, float want, const char *label)
{
    const float got = cupcake_game_tick_rate_sec(p);

    if (!near(got, want)) {
        fprintf(stderr, "FAIL: %s expected %.6f got %.6f\n", label, want, got);
        g_fail = 1;
    }
}

static void test_rate_level1_phases(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.level = 1;
    p.phase_threshold = 10000;

    expect_rate(&p, 26.f / 30.f, "level1 phase1");
    p.phase = 2;
    expect_rate(&p, 23.f / 30.f, "level1 phase2");
    p.phase = 3;
    expect_rate(&p, 21.f / 30.f, "level1 phase3");
    p.phase = 4;
    expect_rate(&p, 19.f / 30.f, "level1 phase4");
    p.phase = 5;
    expect_rate(&p, 17.f / 30.f, "level1 phase5");
    p.phase = 6;
    expect_rate(&p, 15.f / 30.f, "level1 phase6");
}

static void test_rate_level2_phases(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.level = 2;
    p.phase_threshold = 10000;

    expect_rate(&p, 19.f / 30.f, "level2 phase1");
    p.phase = 6;
    expect_rate(&p, 9.f / 30.f, "level2 phase6");
}

static void test_rate_slowdown_near_threshold(void)
{
    cupcake_play_state_t p;
    const float base = 26.f / 30.f;
    const float slow = base - base * 0.15f;

    memset(&p, 0, sizeof p);
    p.level = 1;
    p.phase = 1;
    p.phase_threshold = 10000;

    p.points = 8999;
    expect_rate(&p, base, "slowdown: below 90%");
    p.points = 9000;
    expect_rate(&p, slow, "slowdown: at 90%");
    p.points = 9999;
    expect_rate(&p, slow, "slowdown: below phase target");
    p.points = 10000;
    expect_rate(&p, base, "slowdown: at phase target (no slowdown)");
}

static void test_rate_no_slowdown_at_debug_threshold(void)
{
    cupcake_play_state_t p;
    const float base = 26.f / 30.f;

    memset(&p, 0, sizeof p);
    p.level = 1;
    p.phase = 1;
    p.phase_threshold = 1000;
    p.points = 950;
    expect_rate(&p, base, "debug threshold 1000: slowdown disabled");
}

static void setup_play(void)
{
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_start(1, 0);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
}

static void test_game_timer_advances_ticks(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    float rate;

    setup_play();
    p = cupcake_play_state();
    tm = cupcake_timers();
    rate = cupcake_game_tick_rate_sec(p);

    if (p->game_tick != 0)
        fail("game timer: no OT until first interval (start_tick 0)");
    if (cupcake_timer_tick(tm, CUPCAKE_TMR_GAME) != 1)
        fail("game timer: internal tick 1 after immediate start fire");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_GAME))
        fail("game timer: active during play");

    cupcake_timers_update(tm, rate);
    if (p->game_tick != 1)
        fail("game timer: OT(1) after one interval");

    cupcake_timers_update(tm, rate);
    if (p->game_tick != 2)
        fail("game timer: OT(2) after second interval");
}

static void test_entity_steps_each_tick(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    float rate;
    uint16_t pac_next;
    uint16_t couch_next;

    setup_play();
    p = cupcake_play_state();
    tm = cupcake_timers();
    rate = cupcake_game_tick_rate_sec(p);

    pac_next = 50;
    couch_next = 60;
    p->pacifier.next = pac_next;
    p->couch.next = couch_next;

    cupcake_timers_update(tm, rate);
    if (p->pacifier.next != pac_next - 1)
        fail("entity step: pacifier.next decremented");
    if (p->couch.next != couch_next - 1)
        fail("entity step: couch.next decremented");
}

int main(void)
{
    test_rate_level1_phases();
    test_rate_level2_phases();
    test_rate_slowdown_near_threshold();
    test_rate_no_slowdown_at_debug_threshold();
    test_game_timer_advances_ticks();
    test_entity_steps_each_tick();

    if (g_fail)
        return 1;
    printf("cupcake_game_tick: all tests passed\n");
    return 0;
}
