/*
 * TASK-27 Maggie throw cycle — run: make test-maggie-step
 */
#include "cupcake.h"

#include <stdio.h>
#include <string.h>

static int g_fail;
static char g_sfx[8][16];
static int g_sfx_count;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static int test_cb(cupcake_cb_type_t type, const char *str, int a0, int a1)
{
    (void)a0;
    (void)a1;
    if (type == CUPCAKE_CB_SFX && str && g_sfx_count < 8) {
        snprintf(g_sfx[g_sfx_count], sizeof g_sfx[0], "%s", str);
        g_sfx_count++;
    }
    return 0;
}

static int last_sfx_is(const char *id)
{
    if (g_sfx_count <= 0)
        return 0;
    return strcmp(g_sfx[g_sfx_count - 1], id) == 0;
}

static void reset_sfx(void)
{
    g_sfx_count = 0;
}

static void test_maggie_start_resets(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.maggie.loop = 3;
    p.maggie.index = 2;
    p.maggie.visible = 1;

    cupcake_maggie_start(&p);

    if (p.maggie.loop != 0 || p.maggie.index != 0 || p.maggie.visible)
        fail("maggie start: resets loop/index/visible");
}

static void test_maggie_start_with_step(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_maggie_start(&p);
    cupcake_maggie_step(&p);

    if (!p.maggie.visible || p.maggie.index != 0 || p.maggie.loop != 1)
        fail("maggie start step_now: first pose index 0");
}

static void test_throw_cycle_indices(void)
{
    cupcake_play_state_t p;
    int i;

    memset(&p, 0, sizeof p);
    for (i = 0; i < 4; i++) {
        cupcake_maggie_step(&p);
        if (p.maggie.index != (uint8_t)i)
            fail("maggie step: cycles maggie0-3");
        if (!p.maggie.visible)
            fail("maggie step: visible during cycle");
    }
    if (p.maggie.loop != 0)
        fail("maggie step: loop wraps to 0");
}

static void test_throw_sfx_when_loop_gt0(void)
{
    cupcake_play_state_t p;

    cupcake_set_callback(test_cb);
    memset(&p, 0, sizeof p);
    reset_sfx();

    cupcake_maggie_step(&p);
    if (g_sfx_count != 0)
        fail("throw sfx: silent on loop 0");

    cupcake_maggie_step(&p);
    if (!last_sfx_is("throw"))
        fail("throw sfx: plays on loop 1");
}

static void test_loop3_shows_cake8(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.maggie.loop = 3;
    cupcake_maggie_step(&p);

    if (!cupcake_aircake_is_visible(&p.aircakes, 8))
        fail("loop 3: shows aircake8");
    if (p.maggie.index != 3)
        fail("loop 3: maggie index 3");
}

static void test_couch_slot3_suppresses(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.maggie.loop = 2;
    p.maggie.visible = 1;
    cupcake_couch_set_frame_visible(&p.couch, 3, 1);

    cupcake_maggie_step(&p);

    if (p.maggie.visible)
        fail("couch3: maggie hidden");
    if (p.maggie.loop != 0)
        fail("couch3: loop reset");
}

static void test_index_reset_scheduled(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    float rate;

    cupcake_set_callback(test_cb);
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();
    tm = cupcake_timers();

    p->maggie.loop = 3;
    cupcake_maggie_step(p);
    if (p->maggie.index != 3)
        fail("schedule: index 3 before timer");

    rate = cupcake_game_tick_rate_sec(p);
    cupcake_timers_update(tm, 0.75f * rate - 0.01f);
    if (p->maggie.index != 3)
        fail("schedule: index held before delay");
    cupcake_timers_update(tm, 0.02f);
    if (p->maggie.index != 0)
        fail("schedule: index reset after 0.75*game.rate");
}

int main(void)
{
    test_maggie_start_resets();
    test_maggie_start_with_step();
    test_throw_cycle_indices();
    test_throw_sfx_when_loop_gt0();
    test_loop3_shows_cake8();
    test_couch_slot3_suppresses();
    test_index_reset_scheduled();

    if (g_fail)
        return 1;
    printf("cupcake_maggie_step: all tests passed\n");
    return 0;
}
