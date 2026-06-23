/*
 * TASK-47 demo → start intro — run: make test-demo-start
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

static void test_demo_action_enters_start(void)
{
    const cupcake_state_t *st;

    cupcake_init();
    press_button(CUPCAKE_BTN_SELECT);

    press_button(CUPCAKE_BTN_ACTION);
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_START)
        fail("demo+Action: must enter start mode, not play");
    if (st->play.enabled)
        fail("demo+Action: input disabled during start intro");
    if (!cupcake_timer_active(cupcake_timers(), CUPCAKE_TMR_START))
        fail("demo+Action: start timer must run");
}

static void test_quick_start_enters_start(void)
{
    const cupcake_state_t *st;

    cupcake_init();
    press_button(CUPCAKE_BTN_LEVEL1);

    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_START)
        fail("quick start: must enter start mode");
    if (st->play.mode == CUPCAKE_MODE_PLAY)
        fail("quick start: must not skip to play");
    if (st->play.level != 1)
        fail("quick start: level 1 set");
}

static void test_play_only_after_start_timer(void)
{
    const cupcake_state_t *st;
    cupcake_timers_t *tm;

    cupcake_init();
    press_button(CUPCAKE_BTN_SELECT);
    press_button(CUPCAKE_BTN_ACTION);

    st = cupcake_get_state();
    if (st->play.mode == CUPCAKE_MODE_PLAY)
        fail("before intro: must not be in play");

    tm = cupcake_timers();
    cupcake_timers_update(tm, 3.16f);
    st = cupcake_get_state();
    if (st->play.mode == CUPCAKE_MODE_PLAY)
        fail("after tick1: still in start intro");

    cupcake_timers_update(tm, 3.16f);
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_PLAY)
        fail("after tick2: onPhaseStart enters play");
    if (!st->play.enabled)
        fail("after tick2: play enabled");
}

int main(void)
{
    test_demo_action_enters_start();
    test_quick_start_enters_start();
    test_play_only_after_start_timer();

    if (g_fail)
        return 1;
    printf("cupcake_demo_start: all tests passed\n");
    return 0;
}
