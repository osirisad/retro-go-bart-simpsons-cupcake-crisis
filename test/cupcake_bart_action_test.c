/*
 * TASK-19 Bart action throw toward Marge — run: make test-bart-action
 */
#include "cupcake.h"

#include <math.h>
#include <stdio.h>

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

static void press_action(void)
{
    cupcake_set_buttons(CUPCAKE_BTN_ACTION);
    cupcake_update();
    cupcake_set_buttons(0);
}

static void enter_play(cupcake_play_state_t **out)
{
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    *out = cupcake_play_state();
}

static void run_action_timer(cupcake_timers_t *tm, float rate)
{
    cupcake_timers_update(tm, rate);
}

static void test_action_only_at_lane1(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    enter_play(&p);
    tm = cupcake_timers();
    if (p->bart.pos != 2)
        fail("setup: lane 2");
    press_action();
    if (cupcake_timer_active(tm, CUPCAKE_TMR_ACTION))
        fail("action: ignored when not lane 1");
}

static void test_action_animates_to_zero_then_back(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    float rate;

    enter_play(&p);
    tm = cupcake_timers();
    cupcake_bart_set_position(p, 1);
    rate = cupcake_game_tick_rate_sec(p) * 0.8f;

    press_action();
    if (p->bart.pos != 0)
        fail("action onStart: position 0");
    if (!cupcake_bart_material_visible(p, 0))
        fail("action onStart: bart0 visible");

    run_action_timer(tm, rate);
    if (p->bart.pos != 1)
        fail("action onEnd: returned to lane 1");
    if (!cupcake_bart_material_visible(p, 1))
        fail("action onEnd: bart1 visible");
}

static void test_action_rate_without_marge(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    const float want = (26.f / 30.f) * 0.8f;

    enter_play(&p);
    tm = cupcake_timers();
    p->marge.visible = 0;
    cupcake_bart_set_position(p, 1);
    press_action();
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_ACTION))
        fail("action: timer started");
    if (!near(tm->named[CUPCAKE_TMR_ACTION].cfg.rate_sec, want))
        fail("action: rate = game * 0.8 without Marge");
}

static void test_action_rate_with_marge(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    const float want = (26.f / 30.f) * 0.5f;

    enter_play(&p);
    tm = cupcake_timers();
    p->marge.visible = 1;
    cupcake_bart_set_position(p, 1);
    press_action();
    if (!near(tm->named[CUPCAKE_TMR_ACTION].cfg.rate_sec, want))
        fail("action: rate = game * 0.5 with Marge on-screen");
}

static void test_action_marge_collect_count_reset(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    float rate;

    enter_play(&p);
    tm = cupcake_timers();
    p->marge.visible = 1;
    p->bart.count = 3;
    cupcake_bart_set_position(p, 1);
    rate = cupcake_game_tick_rate_sec(p) * 0.5f;

    press_action();
    run_action_timer(tm, rate);
    if (p->bart.count != 1)
        fail("action collect: count 1 when stack slot 1 visible on lane 1");
}

static void test_action_marge_collect_floor_cupcake(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    float rate;

    enter_play(&p);
    tm = cupcake_timers();
    p->marge.visible = 1;
    cupcake_grid_set_visible(&p->grid, 1, 1, 1);
    p->bart.count = 2;
    cupcake_bart_set_position(p, 1);
    rate = cupcake_game_tick_rate_sec(p) * 0.5f;

    press_action();
    run_action_timer(tm, rate);
    if (p->bart.count != 1)
        fail("action collect: count 1 when floor cupcake remains at lane 1");
}

static void test_action_no_miss_without_marge(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    enter_play(&p);
    tm = cupcake_timers();
    p->marge.visible = 0;
    p->bart.count = 2;
    cupcake_bart_set_position(p, 1);
    cupcake_grid_set_visible(&p->grid, 1, 1, 1);
    cupcake_grid_set_visible(&p->grid, 1, 2, 1);

    press_action();
    if (p->bart.pos != 0)
        fail("action throw: bart at position 0");

    cupcake_timers_update(tm, cupcake_game_tick_rate_sec(p));
    if (cupcake_timer_active(tm, CUPCAKE_TMR_MISS))
        fail("action without marge: no miss during throw");
    if (p->bart.count != 2)
        fail("action without marge: cupcake count unchanged after game tick");
}

static void test_action_no_collect_without_marge(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    float rate;

    enter_play(&p);
    tm = cupcake_timers();
    p->marge.visible = 0;
    p->bart.count = 2;
    cupcake_bart_set_position(p, 1);
    rate = cupcake_game_tick_rate_sec(p) * 0.8f;

    press_action();
    run_action_timer(tm, rate);
    if (p->bart.count != 2)
        fail("action: count unchanged when Marge not visible");
}

int main(void)
{
    test_action_only_at_lane1();
    test_action_animates_to_zero_then_back();
    test_action_rate_without_marge();
    test_action_rate_with_marge();
    test_action_marge_collect_count_reset();
    test_action_marge_collect_floor_cupcake();
    test_action_no_miss_without_marge();
    test_action_no_collect_without_marge();

    if (g_fail)
        return 1;
    printf("cupcake_bart_action: all tests passed\n");
    return 0;
}
