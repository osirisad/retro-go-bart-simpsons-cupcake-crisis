/*
 * TASK-23 Cupcakes grid step miss detection — run: make test-cupcakes-step
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

static void test_cupcakes_start_hides_grid(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_grid_set_visible(&p.grid, 2, 1, 1);
    cupcake_grid_set_visible(&p.grid, 4, 3, 1);
    cupcake_grid_set_visible(&p.grid, 5, 5, 1);

    cupcake_cupcakes_start(&p);

    if (p.grid.group_visible)
        fail("cupcakes start: group hidden");
    if (p.grid.visible != 0)
        fail("cupcakes start: all cells cleared");
    if (cupcake_grid_is_visible(&p.grid, 5, 5))
        fail("cupcakes start: lane5 row cleared");
}

static void test_step_no_miss_when_bart_in_lane(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.bart.pos = 2;
    cupcake_grid_set_visible(&p.grid, 2, 1, 1);

    cupcake_cupcakes_step(&p);

    if (!cupcake_grid_is_visible(&p.grid, 2, 1))
        fail("step: floor cupcake kept when bart in lane");
}

static void test_step_no_miss_on_couch(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.bart.pos = 5;
    cupcake_grid_set_visible(&p.grid, 3, 1, 1);

    cupcake_cupcakes_step(&p);

    if (!cupcake_grid_is_visible(&p.grid, 3, 1))
        fail("step: floor cupcake kept when bart on couch");
}

static void test_step_no_miss_when_bart_throwing(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.bart.pos = 0;
    p.bart.count = 2;
    cupcake_grid_set_visible(&p.grid, 1, 1, 1);
    cupcake_grid_set_visible(&p.grid, 1, 2, 1);

    cupcake_cupcakes_step(&p);

    if (!cupcake_grid_is_visible(&p.grid, 1, 1))
        fail("step: held cupcakes kept when bart throwing to Marge");
}

static void test_step_miss_other_lane(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    cupcake_bart_set_position(p, 2);
    cupcake_grid_set_visible(&p->grid, 3, 1, 1);

    cupcake_cupcakes_step(p);

    if (cupcake_grid_is_visible(&p->grid, 3, 1))
        fail("step miss: floor slot cleared");
    if (!cupcake_is_paused())
        fail("step miss: game paused");
}

static void test_step_only_slot1_misses(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.bart.pos = 1;
    cupcake_grid_set_visible(&p.grid, 3, 2, 1);

    cupcake_cupcakes_step(&p);

    if (!cupcake_grid_is_visible(&p.grid, 3, 2))
        fail("step: upper stack slot not treated as floor miss");
}

int main(void)
{
    test_cupcakes_start_hides_grid();
    test_step_no_miss_when_bart_in_lane();
    test_step_no_miss_on_couch();
    test_step_no_miss_when_bart_throwing();
    test_step_miss_other_lane();
    test_step_only_slot1_misses();

    if (g_fail)
        return 1;
    printf("cupcake_cupcakes_step: all tests passed\n");
    return 0;
}
