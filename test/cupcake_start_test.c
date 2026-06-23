/*
 * TASK-04 start mode — run: make test-start
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

static void test_start_sequence(void)
{
    const cupcake_state_t *st;
    cupcake_timers_t *tm;

    cupcake_init();
    tm = cupcake_timers();

    cupcake_play_state()->level = 1;
    cupcake_play_state()->scoreboard.hi_score[1] = 12345;

    cupcake_on_start(1, 0);

    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_START)
        fail("start: mode after on_start");
    if (st->play.bart.pos != 2 || st->play.bart.count != 0)
        fail("start: bart reset on timer onStart");
    if (st->play.points != 0)
        fail("start: points cleared on timer onStart");
    if (st->play.miss.count != 0)
        fail("start: miss counter cleared");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_START))
        fail("start: start timer active");

    cupcake_timers_update(tm, 3.16f);
    st = cupcake_get_state();
    if (st->play.scoreboard.score != 12345u)
        fail("start: tick1 shows level hi-score");

    cupcake_timers_update(tm, 3.16f);
    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_PLAY)
        fail("start: play mode after tick2");
    if (!st->play.enabled)
        fail("start: enabled after onPhaseStart");
    if (st->play.scoreboard.value != 0)
        fail("start: tick2 clears scoreboard.value");
    if (cupcake_timer_active(tm, CUPCAKE_TMR_START))
        fail("start: timer finished after 2 ticks");
}

int main(void)
{
    test_start_sequence();

    if (g_fail)
        return 1;
    printf("cupcake_start: all tests passed\n");
    return 0;
}
