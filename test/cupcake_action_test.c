/*
 * TASK-48 play Action debug removal — run: make test-action
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
    cupcake_init();
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_PLAY ||
        !cupcake_get_state()->play.enabled)
        fail("setup: play mode");
}

static void test_play_action_no_free_cupcakes(void)
{
    cupcake_play_state_t *p;
    uint8_t count_before;
    uint32_t points_before;
    int i;

    enter_play();
    p = cupcake_play_state();
    p->bart.count = 2;
    p->points = 500;
    p->scoreboard.score = 500;
    count_before = p->bart.count;
    points_before = p->points;

    for (i = 0; i < 5; i++)
        press_button(CUPCAKE_BTN_ACTION);

    if (p->bart.count != count_before)
        fail("play Action: must not cycle bart count");
    if (p->points != points_before)
        fail("play Action: must not award free points");
    if (p->scoreboard.score != points_before)
        fail("play Action: must not change scoreboard score");
}

static void test_play_action_ignored_when_paused(void)
{
    cupcake_play_state_t *p;

    enter_play();
    p = cupcake_play_state();
    p->bart.count = 1;
    p->points = 200;
    cupcake_pause();

    press_button(CUPCAKE_BTN_ACTION);

    if (p->bart.count != 1)
        fail("paused Action: count unchanged");
    if (p->points != 200)
        fail("paused Action: points unchanged");
}

int main(void)
{
    test_play_action_no_free_cupcakes();
    test_play_action_ignored_when_paused();

    if (g_fail)
        return 1;
    printf("cupcake_action: all tests passed\n");
    return 0;
}
