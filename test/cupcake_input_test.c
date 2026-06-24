/*
 * TASK-07 input router — run: make test-input
 */
#include "cupcake.h"

#include <stdio.h>

static int g_fail;
static int g_sound_toggles;

static int test_cb(cupcake_cb_type_t type, const char *str, int a0, int a1)
{
    (void)str;
    (void)a0;
    (void)a1;
    if (type == CUPCAKE_CB_SOUND_TOGGLE)
        g_sound_toggles++;
    return 0;
}

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
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_PLAY)
        fail("setup: enter play");
}

static void test_select_level_cycle(void)
{
    const cupcake_state_t *st;

    cupcake_init();
    press_button(CUPCAKE_BTN_SELECT);
    st = cupcake_get_state();
    if (st->play.level != 1)
        fail("select: level 0 -> 1");

    press_button(CUPCAKE_BTN_SELECT);
    st = cupcake_get_state();
    if (st->play.level != 2)
        fail("select: level 1 -> 2");

    press_button(CUPCAKE_BTN_SELECT);
    st = cupcake_get_state();
    if (st->play.level != 0 || st->play.mode != CUPCAKE_MODE_DEMO)
        fail("select: level 2 -> 0 restores demo");
}

static void test_quick_start(void)
{
    cupcake_init();
    press_button(CUPCAKE_BTN_LEVEL2);
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_START)
        fail("quick start: enters start mode");
    if (cupcake_play_state()->level != 2)
        fail("quick start: level 2");
}

static void test_move_only_when_enabled(void)
{
    const cupcake_state_t *st;

    enter_play();
    cupcake_pause();
    press_button(CUPCAKE_BTN_RIGHT);
    if (cupcake_get_state()->play.bart.pos != 2)
        fail("move: ignored while paused");

    cupcake_resume();
    press_button(CUPCAKE_BTN_RIGHT);
    st = cupcake_get_state();
    if (st->play.bart.pos != 3)
        fail("move: right when enabled");
}

static void test_sound_toggle_on_release(void)
{
    cupcake_set_callback(test_cb);
    cupcake_init();
    g_sound_toggles = 0;

    cupcake_set_buttons(CUPCAKE_BTN_SOUND);
    cupcake_update();
    if (g_sound_toggles != 0)
        fail("sound: no toggle on press");

    cupcake_set_buttons(0);
    cupcake_update();
    if (g_sound_toggles != 1)
        fail("sound: toggle on release");
}

static void test_action_con_in_demo(void)
{
    cupcake_init();
    cupcake_play_state()->mode = CUPCAKE_MODE_DEMO;
    cupcake_play_state()->phase = 4;
    cupcake_play_state()->level = 1;
    cupcake_play_state()->scoreboard.show_con = 1;

    press_button(CUPCAKE_BTN_ACTION);
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_START)
        fail("action: CON continues via start mode");
    if (cupcake_play_state()->phase != 4)
        fail("action: CON preserves phase");
}

int main(void)
{
    test_select_level_cycle();
    test_quick_start();
    test_move_only_when_enabled();
    test_sound_toggle_on_release();
    test_action_con_in_demo();

    if (g_fail)
        return 1;
    printf("cupcake_input: all tests passed\n");
    return 0;
}
