/*
 * TASK-05 over mode — run: make test-over
 */
#include "cupcake.h"

#include <stdio.h>
#include <string.h>

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

static void test_game_over(void)
{
    const cupcake_state_t *st;
    cupcake_timer_config_t cfg;

    cupcake_init();
    cupcake_play_state()->mode = CUPCAKE_MODE_PLAY;
    cupcake_play_state()->enabled = 1;
    cupcake_play_state()->level = 1;
    cupcake_play_state()->phase = 4;
    cupcake_play_state()->bart.count = 3;
    cupcake_play_state()->miss.count = 2;

    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 1.f;
    cfg.max_ticks = 0;
    cupcake_timer_start(cupcake_timers(), CUPCAKE_TMR_GAME, &cfg);
    if (!cupcake_timer_active(cupcake_timers(), CUPCAKE_TMR_GAME))
        fail("over: setup game timer");

    cupcake_on_game_over();
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_OVER)
        fail("over: mode");
    if (st->play.enabled)
        fail("over: disabled input");
    if (st->play.bart.count != 0)
        fail("over: bart count cleared");
    if (cupcake_timer_active(cupcake_timers(), CUPCAKE_TMR_GAME))
        fail("over: timers stopped");
}

static void test_select_con_continue(void)
{
    const cupcake_state_t *st;

    cupcake_init();
    cupcake_play_state()->mode = CUPCAKE_MODE_PLAY;
    cupcake_play_state()->enabled = 1;
    cupcake_play_state()->level = 2;
    cupcake_play_state()->phase = 3;
    cupcake_play_state()->miss.count = 3;

    cupcake_on_game_over();
    press_button(CUPCAKE_BTN_SELECT);

    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_DEMO)
        fail("over: Select returns to demo");
    if (!st->play.scoreboard.show_con)
        fail("over: Select sets CON");
    if (st->play.level != 2)
        fail("over: level preserved");

    press_button(CUPCAKE_BTN_ACTION);
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_START)
        fail("over: Action with CON enters start");
    if (st->play.phase != 3)
        fail("over: continue at current phase");
    if (st->play.scoreboard.show_con)
        fail("over: CON cleared on restart");
}

static void test_miss_increase_triggers_over(void)
{
    const cupcake_state_t *st;
    int i;

    cupcake_init();
    cupcake_play_state()->mode = CUPCAKE_MODE_PLAY;
    cupcake_play_state()->enabled = 1;
    cupcake_play_state()->miss.count = 0;

    for (i = 0; i < 2; i++)
        cupcake_miss_increase();
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_PLAY || st->play.miss.count != 2)
        fail("over: first two misses stay in play");

    cupcake_miss_increase();
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_OVER || st->play.miss.count != 3)
        fail("over: third miss triggers game over");
}

int main(void)
{
    test_game_over();
    test_select_con_continue();
    test_miss_increase_triggers_over();

    if (g_fail)
        return 1;
    printf("cupcake_over: all tests passed\n");
    return 0;
}
