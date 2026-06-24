/*
 * TASK-42 save/load via game API — run: make test-save-game
 */
#include "cupcake.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void enter_play(void)
{
    cupcake_timers_t *tm = cupcake_timers();

    cupcake_on_quick_start(1);
    cupcake_timers_update(tm, 3.16f);
    cupcake_timers_update(tm, 3.16f);
}

static void test_play_roundtrip(void)
{
    cupcake_play_state_t *p;
    void *buf;

    cupcake_init();
    enter_play();
    p = cupcake_play_state();
    cupcake_grid_set_visible(&p->grid, 2, 3, 1);
    p->couch.next = 42;
    p->miss.count = 1;
    p->points = 1234u;
    p->scoreboard.score = p->points;

    buf = malloc(cupcake_state_size());
    if (!buf)
        fail("malloc save buffer");
    cupcake_save_state(buf);

    p->couch.next = 0;
    p->miss.count = 0;
    p->points = 0;
    cupcake_grid_clear(&p->grid);

    if (!cupcake_load_state(buf))
        fail("load_state: rejected blob");

    p = cupcake_play_state();
    if (p->mode != CUPCAKE_MODE_PLAY)
        fail("load: play mode");
    if (p->couch.next != 42)
        fail("load: couch.next");
    if (p->miss.count != 1)
        fail("load: miss count");
    if (p->points != 1234u)
        fail("load: points");
    if (!cupcake_grid_is_visible(&p->grid, 2, 3))
        fail("load: grid bit");

    free(buf);
}

static void test_mid_miss_resume(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    void *buf;

    cupcake_init();
    enter_play();
    p = cupcake_play_state();
    tm = cupcake_timers();

    cupcake_on_m_cupcake(2);
    if (!cupcake_is_paused())
        fail("mid-miss: paused during sequence");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_MISS))
        fail("mid-miss: miss timer active");

    buf = malloc(cupcake_state_size());
    cupcake_save_state(buf);
    cupcake_timer_stop(tm, CUPCAKE_TMR_MISS);
    p->miss.count = 0;

    if (!cupcake_load_state(buf))
        fail("mid-miss load: rejected");

    p = cupcake_play_state();
    tm = cupcake_timers();
    if (!cupcake_is_paused())
        fail("mid-miss load: still paused");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_MISS))
        fail("mid-miss load: miss timer restored");

    free(buf);
}

int main(void)
{
    test_play_roundtrip();
    test_mid_miss_resume();

    if (g_fail)
        return 1;
    printf("cupcake_save_game: all tests passed\n");
    return 0;
}
