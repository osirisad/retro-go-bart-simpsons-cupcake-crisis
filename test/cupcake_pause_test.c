/*
 * TASK-06 pause/resume — run: make test-pause
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

static void setup_play_with_game_timer(void)
{
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_start(1, 0);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_PLAY)
        fail("setup: expected play mode");
}

static void test_pause_blocks_game_timer(void)
{
    cupcake_timers_t *tm;
    uint32_t tick_before;

    setup_play_with_game_timer();
    tm = cupcake_timers();
    tick_before = cupcake_get_state()->play.game_tick;

    cupcake_pause();
    if (!cupcake_is_paused())
        fail("pause: is_paused");
    if (cupcake_get_state()->play.enabled)
        fail("pause: enabled cleared");
    if (!cupcake_timer_is_paused(tm, CUPCAKE_TMR_GAME))
        fail("pause: game timer slot paused");

    cupcake_timers_update(tm, 5.f);
    if (cupcake_get_state()->play.game_tick != tick_before)
        fail("pause: game OT should not advance");

    cupcake_resume();
    if (cupcake_is_paused())
        fail("resume: no longer paused");
    cupcake_timers_update(tm, 26.f / 30.f);
    if (cupcake_get_state()->play.game_tick <= tick_before)
        fail("resume: game OT advances again");
}

static void test_miss_sequence_runs_while_game_paused(void)
{
    cupcake_timers_t *tm;

    setup_play_with_game_timer();
    tm = cupcake_timers();

    cupcake_on_m_cupcake(1);
    if (!cupcake_is_paused() || !cupcake_timer_is_paused(tm, CUPCAKE_TMR_GAME))
        fail("miss cupcake: game paused at start");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_MISS))
        fail("miss cupcake: miss timer running");

    cupcake_timers_update(tm, 0.5f);
    if (cupcake_timer_active(tm, CUPCAKE_TMR_MISS))
        fail("miss cupcake: miss timer should finish");
    if (!cupcake_is_paused())
        fail("miss cupcake: still paused after miss timer (before onM_)");
}

static void test_phase_restart_interstitial(void)
{
    cupcake_timers_t *tm;

    setup_play_with_game_timer();
    tm = cupcake_timers();
    cupcake_play_state()->miss.count = 1;

    cupcake_on_phase_restart();
    if (cupcake_get_state()->play.enabled)
        fail("phase restart: paused during interstitial");
    if (!cupcake_timer_is_paused(tm, CUPCAKE_TMR_GAME))
        fail("phase restart: game timer paused");

    cupcake_timers_update(tm, 0.75f);
    if (cupcake_is_paused())
        fail("phase restart: resume after 0.75s schedule");
    if (!cupcake_get_state()->play.enabled)
        fail("phase restart: enabled restored");
}

static void test_sit_bonus_and_add_bonus_pause(void)
{
    cupcake_timers_t *tm;
    const cupcake_state_t *st;

    setup_play_with_game_timer();
    tm = cupcake_timers();

    cupcake_bart_sit_bonus(2);
    if (!cupcake_is_paused())
        fail("sit bonus: paused during 2s timer");

    cupcake_timers_update(tm, 1.f);
    cupcake_timers_update(tm, 1.01f);
    st = cupcake_get_state();
    if (!st->play.scoreboard.bonus_active)
        fail("sit bonus: addBonus should start after sit timer");
    if (!cupcake_is_paused())
        fail("addBonus: paused during point ticks");

    cupcake_timers_update(tm, 0.03f);
    cupcake_timers_update(tm, 0.03f);
    if (cupcake_get_state()->play.scoreboard.bonus_active)
        fail("addBonus: bonus should finish");
    if (cupcake_is_paused())
        fail("addBonus: resumed after ticks complete");
    if (!cupcake_get_state()->play.enabled)
        fail("addBonus: enabled restored at end");
}

int main(void)
{
    test_pause_blocks_game_timer();
    test_miss_sequence_runs_while_game_paused();
    test_phase_restart_interstitial();
    test_sit_bonus_and_add_bonus_pause();

    if (g_fail)
        return 1;
    printf("cupcake_pause: all tests passed\n");
    return 0;
}
