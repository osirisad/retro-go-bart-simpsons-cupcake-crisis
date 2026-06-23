/*
 * TASK-28 Marge appear / hide stepping — run: make test-marge-step
 */
#include "cupcake.h"
#include "cupcake_rng.h"
#include "cupcake_state.h"

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

static void test_marge_start_resets(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.marge.visible = 1;
    p.marge.loop = 2;

    cupcake_marge_start(&p);

    if (p.marge.visible || p.marge.loop != 0)
        fail("marge start: clears visible and loop");
}

static void test_appear_on_tick_mod6_eq5(void)
{
    cupcake_play_state_t p;

    cupcake_set_callback(test_cb);
    memset(&p, 0, sizeof p);
    p.bart.count = 2;
    reset_sfx();

    cupcake_marge_step(&p, 11);

    if (!p.marge.visible)
        fail("appear: tick 11 (%6==5) shows marge");
    if (!last_sfx_is("marge"))
        fail("appear: marge SFX");
}

static void test_no_appear_without_cupcakes(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_marge_step(&p, 11);
    if (p.marge.visible)
        fail("appear: blocked when bart count 0");
}

static void test_no_appear_when_tick_le6(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.bart.count = 1;
    cupcake_marge_step(&p, 5);
    if (p.marge.visible)
        fail("appear: blocked when tick <= 6");
}

static void test_no_appear_when_couch_onscreen(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.bart.count = 1;
    p.couch.onscreen = 1;
    cupcake_marge_step(&p, 11);
    if (p.marge.visible)
        fail("appear: blocked when couch on-screen");
}

static void test_appear_on_tick_mod6_eq0_seeded(void)
{
    cupcake_play_state_t p;

    cupcake_set_callback(test_cb);
    cupcake_rng_seed(0xABCDEF01u);
    memset(&p, 0, sizeof p);
    p.bart.count = 1;
    reset_sfx();

    cupcake_marge_step(&p, 12);

    if (!p.marge.visible)
        fail("appear: tick 12 with rand(3)==0 shows marge");
}

static void test_onscreen_hides_after_loop2(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.marge.visible = 1;
    p.marge.loop = 0;

    cupcake_marge_step(&p, 1);
    if (!p.marge.visible || p.marge.loop != 1)
        fail("onscreen: loop 0 -> 1");

    cupcake_marge_step(&p, 2);
    if (!p.marge.visible || p.marge.loop != 2)
        fail("onscreen: loop 1 -> 2");

    cupcake_marge_step(&p, 3);
    if (p.marge.visible || p.marge.loop != 0)
        fail("onscreen: hides when loop reaches 2");
}

int main(void)
{
    test_marge_start_resets();
    test_appear_on_tick_mod6_eq5();
    test_no_appear_without_cupcakes();
    test_no_appear_when_tick_le6();
    test_no_appear_when_couch_onscreen();
    test_appear_on_tick_mod6_eq0_seeded();
    test_onscreen_hides_after_loop2();

    if (g_fail)
        return 1;
    printf("cupcake_marge_step: all tests passed\n");
    return 0;
}
