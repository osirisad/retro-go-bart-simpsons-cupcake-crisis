/*
 * TASK-25 Aircakes step chain — run: make test-aircakes-step
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

static void test_aircakes_start_hides(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_aircake_set_visible(&p.aircakes, 3, 1);
    cupcake_aircake_set_visible(&p.aircakes, 8, 1);

    cupcake_aircakes_start(&p);

    if (p.aircakes.visible != 0)
        fail("aircakes start: all hidden");
    if (p.aircakes.group_visible)
        fail("aircakes start: group hidden");
}

static void test_cake1_lands_on_grid(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_bart_set_position(&p, 2);
    cupcake_aircake_set_visible(&p.aircakes, 1, 1);

    cupcake_aircakes_step(&p);

    if (cupcake_aircake_is_visible(&p.aircakes, 1))
        fail("cake1 land: flying cake hidden");
    if (!cupcake_grid_is_visible(&p.grid, 2, 1))
        fail("cake1 land: floor slot on lane 1");
}

static void test_cake1_catches_at_bart_lane(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_bart_set_position(&p, 3);
    cupcake_aircake_set_visible(&p.aircakes, 3, 1);

    cupcake_aircakes_step(&p);

    if (p.bart.count != 1)
        fail("cake3 land: catch at bart lane");
    if (cupcake_aircake_is_visible(&p.aircakes, 3))
        fail("cake3 land: flying cake cleared");
}

static void test_cake5_advances_to_cake1(void)
{
    cupcake_play_state_t p;

    cupcake_set_callback(test_cb);
    memset(&p, 0, sizeof p);
    cupcake_aircake_set_visible(&p.aircakes, 5, 1);
    reset_sfx();

    cupcake_aircakes_step(&p);

    if (cupcake_aircake_is_visible(&p.aircakes, 5))
        fail("cake5: hidden after step");
    if (!cupcake_aircake_is_visible(&p.aircakes, 1))
        fail("cake5: advances to cake1");
    if (!last_sfx_is("step"))
        fail("cake5: step SFX");
}

static void test_cake6_pick_seeded(void)
{
    cupcake_play_state_t p;

    cupcake_set_callback(test_cb);
    cupcake_rng_seed(0x12345678u);
    memset(&p, 0, sizeof p);
    cupcake_aircake_set_visible(&p.aircakes, 6, 1);
    reset_sfx();

    cupcake_aircakes_step(&p);

    if (cupcake_aircake_is_visible(&p.aircakes, 6))
        fail("cake6: hidden after step");
    if (!cupcake_aircake_is_visible(&p.aircakes, 2) && !cupcake_aircake_is_visible(&p.aircakes, 5))
        fail("cake6: shows cake2 or cake5");
    if (!last_sfx_is("step"))
        fail("cake6: step SFX");
}

static void test_cake8_phase_branch(void)
{
    cupcake_play_state_t p;

    cupcake_set_callback(test_cb);
    cupcake_rng_seed(0xABCDEF01u);
    memset(&p, 0, sizeof p);
    p.phase = 4;
    cupcake_aircake_set_visible(&p.aircakes, 8, 1);
    reset_sfx();

    cupcake_aircakes_step(&p);

    if (cupcake_aircake_is_visible(&p.aircakes, 8))
        fail("cake8: hidden after step");
    if (!cupcake_aircake_is_visible(&p.aircakes, 4) &&
        !cupcake_aircake_is_visible(&p.aircakes, 7))
        fail("cake8 late: shows cake4 or cake7");
}

static void test_inactive_cakes_unchanged(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_bart_set_position(&p, 2);
    cupcake_aircake_set_visible(&p.aircakes, 2, 1);

    cupcake_aircakes_step(&p);

    if (cupcake_aircake_is_visible(&p.aircakes, 1) || cupcake_aircake_is_visible(&p.aircakes, 3))
        fail("inactive: cakes 1 and 3 stay hidden");
}

int main(void)
{
    test_aircakes_start_hides();
    test_cake1_lands_on_grid();
    test_cake1_catches_at_bart_lane();
    test_cake5_advances_to_cake1();
    test_cake6_pick_seeded();
    test_cake8_phase_branch();
    test_inactive_cakes_unchanged();

    if (g_fail)
        return 1;
    printf("cupcake_aircakes_step: all tests passed\n");
    return 0;
}
