/*
 * TASK-22 Bart miss poses bart6/bart9 — run: make test-bart-miss
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

static void enter_play(cupcake_play_state_t **out)
{
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    *out = cupcake_play_state();
}

static void test_miss_cupcake_shows_bart9(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    enter_play(&p);
    tm = cupcake_timers();
    cupcake_bart_set_position(p, 2);
    p->grid.group_visible = 1;
    cupcake_grid_set_visible(&p->grid, 2, 1, 1);

    cupcake_on_m_cupcake(1);
    cupcake_timers_update(tm, 0.5f);

    if (cupcake_bart_miss_index(p) != 9)
        fail("miss cupcake: bart9 index");
    if (!cupcake_bart_material_visible(p, 9))
        fail("miss cupcake: bart9 visible");
    if (cupcake_bart_material_visible(p, 2))
        fail("miss cupcake: normal pose hidden");
    if (p->grid.group_visible)
        fail("miss cupcake: grid hidden");
    if (!cupcake_aircake_is_visible(&p->aircakes, 0))
        fail("miss cupcake: aircake0 shown for lane 1");
}

static void test_miss_couch_shows_bart6(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    int ticks;

    enter_play(&p);
    tm = cupcake_timers();
    cupcake_bart_set_position(p, 2);
    p->grid.group_visible = 1;
    p->aircakes.group_visible = 1;
    cupcake_aircake_set_visible(&p->aircakes, 3, 1);

    cupcake_on_m_couch();
    ticks = 6 - (int)p->bart.pos;
    cupcake_timers_update(tm, 0.4f * (float)ticks);

    if (cupcake_bart_miss_index(p) != 6)
        fail("miss couch: bart6 index");
    if (!cupcake_bart_material_visible(p, 6))
        fail("miss couch: bart6 visible");
    if (p->grid.group_visible)
        fail("miss couch: grid hidden");
    if (p->aircakes.group_visible)
        fail("miss couch: aircakes hidden");
}

static void test_miss_pose_until_on_m(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    enter_play(&p);
    tm = cupcake_timers();
    cupcake_on_m_cupcake(3);
    cupcake_timers_update(tm, 0.5f);
    if (cupcake_bart_miss_index(p) != 9)
        fail("miss pose: bart9 before onM_");

    cupcake_timers_update(tm, 1.75f);
    if (cupcake_bart_miss_index(p) != -1)
        fail("miss pose: cleared after phase restart");
}

static void test_miss_resume_after_life_lost(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    enter_play(&p);
    tm = cupcake_timers();
    cupcake_bart_set_position(p, 2);
    p->grid.group_visible = 1;
    cupcake_grid_set_visible(&p->grid, 2, 1, 1);

    cupcake_on_m_cupcake(2);
    cupcake_timers_update(tm, 0.5f);
    cupcake_timers_update(tm, 1.75f);
    cupcake_timers_update(tm, 0.75f);

    if (p->miss.count != 1)
        fail("miss resume: one life lost");
    if (cupcake_is_paused() || !p->enabled)
        fail("miss resume: gameplay resumed");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_GAME))
        fail("miss resume: game timer running");
    if (!p->aircakes.group_visible)
        fail("miss resume: aircakes group restored");
}

static void test_bart_start_clears_miss(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_bart_set_miss_pose(&p, 9);
    cupcake_bart_start(&p, 2);
    if (cupcake_bart_miss_index(&p) != -1)
        fail("bart start: clears miss pose");
    if (!cupcake_bart_material_visible(&p, 2))
        fail("bart start: normal pose restored");
}

int main(void)
{
    test_miss_cupcake_shows_bart9();
    test_miss_couch_shows_bart6();
    test_miss_pose_until_on_m();
    test_miss_resume_after_life_lost();
    test_bart_start_clears_miss();

    if (g_fail)
        return 1;
    printf("cupcake_bart_miss: all tests passed\n");
    return 0;
}
