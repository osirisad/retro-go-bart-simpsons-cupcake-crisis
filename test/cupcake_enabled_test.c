/*
 * TASK-57 enabled gating — run: make test-enabled
 */
#include "cupcake.h"

#include <stdio.h>

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void press_button(uint16_t btn)
{
    cupcake_set_buttons(0);
    cupcake_update();
    cupcake_set_buttons(btn);
    cupcake_update();
    cupcake_set_buttons(0);
    cupcake_update();
}

static void enter_play(void)
{
    const cupcake_state_t *st;

    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_PLAY || !st->play.enabled)
        fail("setup: expected active play");
}

static void test_non_play_modes_ignore_move(void)
{
    cupcake_init();
    cupcake_play_state()->enabled = 1;
    press_button(CUPCAKE_BTN_RIGHT);
    if (cupcake_get_state()->play.bart.pos != 2)
        fail("demo: move ignored even if enabled set");

    cupcake_init();
    cupcake_play_state()->mode = CUPCAKE_MODE_OVER;
    cupcake_play_state()->enabled = 1;
    press_button(CUPCAKE_BTN_RIGHT);
    if (cupcake_get_state()->play.bart.pos != 2)
        fail("over: move ignored");

    cupcake_init();
    cupcake_on_start(1, 0);
    if (cupcake_get_state()->play.enabled)
        fail("start: enabled false during intro");
    press_button(CUPCAKE_BTN_RIGHT);
    if (cupcake_get_state()->play.bart.pos != 2)
        fail("start: move ignored");
}

static void test_paused_blocks_move_and_game_ot(void)
{
    cupcake_timers_t *tm;
    uint32_t tick_before;

    enter_play();
    tm = cupcake_timers();
    tick_before = cupcake_get_state()->play.game_tick;

    cupcake_pause();
    if (!cupcake_is_paused())
        fail("pause: is_paused");
    press_button(CUPCAKE_BTN_RIGHT);
    if (cupcake_get_state()->play.bart.pos != 2)
        fail("paused: move ignored");

    cupcake_timers_update(tm, 5.f);
    if (cupcake_get_state()->play.game_tick != tick_before)
        fail("paused: game OT should not advance");
}

static void test_enabled_play_allows_move_and_game_ot(void)
{
    cupcake_timers_t *tm;
    uint32_t tick_before;

    enter_play();
    tm = cupcake_timers();
    tick_before = cupcake_get_state()->play.game_tick;

    press_button(CUPCAKE_BTN_LEFT);
    if (cupcake_get_state()->play.bart.pos != 1)
        fail("enabled play: move left");

    cupcake_timers_update(tm, 26.f / 30.f);
    if (cupcake_get_state()->play.game_tick <= tick_before)
        fail("enabled play: game OT advances");
}

static void test_game_ot_guard_when_enabled_cleared(void)
{
    cupcake_timers_t *tm;
    uint32_t tick_before;

    enter_play();
    tm = cupcake_timers();
    tick_before = cupcake_get_state()->play.game_tick;

    cupcake_play_state()->enabled = 0;
    if (cupcake_timer_is_paused(tm, CUPCAKE_TMR_GAME))
        fail("guard setup: timer still running");

    cupcake_timers_update(tm, 26.f / 30.f);
    if (cupcake_get_state()->play.game_tick != tick_before)
        fail("game OT: must not run when enabled false");
}

static void test_stop_modes_clear_enabled(void)
{
    enter_play();
    cupcake_on_game_over();
    if (cupcake_get_state()->play.enabled)
        fail("game over: enabled cleared");

    cupcake_init();
    cupcake_on_start(1, 0);
    if (cupcake_get_state()->play.enabled)
        fail("start intro: enabled cleared");
}

int main(void)
{
    test_non_play_modes_ignore_move();
    test_paused_blocks_move_and_game_ot();
    test_enabled_play_allows_move_and_game_ot();
    test_game_ot_guard_when_enabled_cleared();
    test_stop_modes_clear_enabled();

    if (g_fail)
        return 1;
    printf("cupcake_enabled: all tests passed\n");
    return 0;
}
