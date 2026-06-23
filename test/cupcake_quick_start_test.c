/*
 * TASK-53 onQuickStart Level1 / Level2 — run: make test-quick-start
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

static void test_level_buttons_from_demo(void)
{
    const cupcake_state_t *st;

    cupcake_init();
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_DEMO)
        fail("setup: starts in demo");

    press_button(CUPCAKE_BTN_LEVEL1);
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_START)
        fail("level1: enters start mode");
    if (st->play.level != 1)
        fail("level1: play.level set");
    if (st->play.scoreboard.level != 1)
        fail("level1: scoreboard.level set");
    if (!cupcake_timer_active(cupcake_timers(), CUPCAKE_TMR_START))
        fail("level1: start intro timer active");

    cupcake_init();
    press_button(CUPCAKE_BTN_LEVEL2);
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_START)
        fail("level2: enters start mode");
    if (st->play.level != 2)
        fail("level2: play.level set");
    if (st->play.scoreboard.level != 2)
        fail("level2: scoreboard.level set");
}

static void test_show_level_not_phase(void)
{
    const cupcake_state_t *st;

    cupcake_init();
    cupcake_play_state()->scoreboard.phase = 6;

    cupcake_on_quick_start(2);
    st = cupcake_get_state();
    if (st->play.scoreboard.level != 2)
        fail("show_level: scoreboard shows level digit");
    if (st->play.scoreboard.phase != 6)
        fail("show_level: must not overwrite scoreboard.phase");

    cupcake_init();
    cupcake_on_start(4, 0);
    st = cupcake_get_state();
    if (st->play.scoreboard.phase != 4)
        fail("normal start: scoreboard shows phase digit");
}

static void test_stops_simulation(void)
{
    cupcake_timers_t *tm;

    cupcake_init();
    tm = cupcake_timers();

    press_button(CUPCAKE_BTN_LEVEL1);
    cupcake_timers_update(tm, 3.16f);
    cupcake_timers_update(tm, 3.16f);
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_PLAY)
        fail("setup: enter play via quick start");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_GAME))
        fail("setup: game timer running in play");

    press_button(CUPCAKE_BTN_LEVEL2);
    if (cupcake_timer_active(tm, CUPCAKE_TMR_GAME))
        fail("quick start: stops game simulation");
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_START)
        fail("quick start: restarts via start intro");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_START))
        fail("quick start: start intro timer active");
    if (cupcake_get_state()->play.level != 2)
        fail("quick start: switches to requested level");
}

int main(void)
{
    test_level_buttons_from_demo();
    test_show_level_not_phase();
    test_stops_simulation();

    if (g_fail)
        return 1;
    printf("cupcake_quick_start: all tests passed\n");
    return 0;
}
