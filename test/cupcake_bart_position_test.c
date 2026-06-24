/*
 * TASK-21 Bart position setter parity — run: make test-bart-position
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

static void expect_material(const cupcake_play_state_t *p, int index, int want, const char *label)
{
    const int got = cupcake_bart_material_visible(p, index);

    if (got != want) {
        fprintf(stderr, "FAIL: %s bart%d expected %d got %d\n", label, index, want, got);
        g_fail = 1;
    }
}

static void expect_only_materials(const cupcake_play_state_t *p, const int *indices, int n,
                                  const char *label)
{
    int i, j, want;

    for (i = 0; i < 10; i++) {
        want = 0;
        for (j = 0; j < n; j++) {
            if (indices[j] == i)
                want = 1;
        }
        if (cupcake_bart_material_visible(p, i) != want) {
            fprintf(stderr, "FAIL: %s unexpected bart%d visibility\n", label, i);
            g_fail = 1;
        }
    }
}

static void test_all_position_layers(void)
{
    cupcake_play_state_t p;
    const int pos0[] = {0, 1};
    const int pos1[] = {1, 7};
    const int pos2[] = {2};
    const int pos3[] = {3};
    const int pos4[] = {4, 8};
    const int pos5[] = {5, 8};

    memset(&p, 0, sizeof p);

    cupcake_bart_set_position(&p, 0);
    expect_only_materials(&p, pos0, 2, "pos0");

    cupcake_bart_set_position(&p, 1);
    expect_only_materials(&p, pos1, 2, "pos1");

    cupcake_bart_set_position(&p, 2);
    expect_only_materials(&p, pos2, 1, "pos2");

    cupcake_bart_set_position(&p, 3);
    expect_only_materials(&p, pos3, 1, "pos3");

    cupcake_bart_set_position(&p, 4);
    expect_only_materials(&p, pos4, 2, "pos4");

    cupcake_bart_set_position(&p, 5);
    expect_only_materials(&p, pos5, 2, "pos5");
}

static void test_hides_prior_layers(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_bart_set_position(&p, 1);
    cupcake_bart_set_position(&p, 3);
    expect_material(&p, 7, 0, "transition 1->3");
    expect_material(&p, 3, 1, "transition 1->3");
}

static void test_stack_lane_mapping(void)
{
    if (cupcake_bart_stack_lane(0) != 1)
        fail("stack lane: pos 0 -> row 0 / lane 1");
    if (cupcake_bart_stack_lane(1) != 2)
        fail("stack lane: pos 1 -> row 1 / lane 2");
    if (cupcake_bart_stack_lane(2) != 3)
        fail("stack lane: pos 2 -> row 2 / lane 3");
    if (cupcake_bart_stack_lane(4) != 5)
        fail("stack lane: pos 4 -> row 4 / lane 5");
    if (cupcake_bart_stack_lane(5) != 5)
        fail("stack lane: pos 5 -> row 4 / lane 5");
}

static void test_stack_moves_with_position(void)
{
    cupcake_play_state_t p;
    int slot;

    memset(&p, 0, sizeof p);
    p.bart.count = 3;
    cupcake_bart_set_position(&p, 4);
    for (slot = 1; slot <= 3; slot++) {
        if (!cupcake_grid_is_visible(&p.grid, 5, slot))
            fail("stack: slots 1-3 on lane 5 at pos 4");
    }
    if (cupcake_grid_is_visible(&p.grid, 5, 4))
        fail("stack: slot 4 hidden when count 3");

    cupcake_bart_set_position(&p, 2);
    if (cupcake_grid_is_visible(&p.grid, 5, 1))
        fail("stack: prior lane 5 cleared");
    for (slot = 1; slot <= 3; slot++) {
        if (!cupcake_grid_is_visible(&p.grid, 3, slot))
            fail("stack: slots 1-3 on lane 3 after move");
    }
}

static void test_stack_on_couch_lane(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.bart.count = 2;
    cupcake_bart_set_position(&p, 5);
    if (!cupcake_grid_is_visible(&p.grid, 5, 1) || !cupcake_grid_is_visible(&p.grid, 5, 2))
        fail("stack: couch pos 5 uses lane 5 (cake41 row)");
}

static void test_move_uses_setter(void)
{
    cupcake_play_state_t *p;

    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();

    p->bart.count = 2;
    cupcake_bart_set_position(p, 2);
    cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (!cupcake_bart_material_visible(p, 3))
        fail("move: lane 3 material after right");
    if (!cupcake_grid_is_visible(&p->grid, 4, 1) || !cupcake_grid_is_visible(&p->grid, 4, 2))
        fail("move: held stack follows lane change");
    if (cupcake_grid_is_visible(&p->grid, 3, 1))
        fail("move: old lane stack hidden");
}

int main(void)
{
    test_all_position_layers();
    test_hides_prior_layers();
    test_stack_lane_mapping();
    test_stack_moves_with_position();
    test_stack_on_couch_lane();
    test_move_uses_setter();

    if (g_fail)
        return 1;
    printf("cupcake_bart_position: all tests passed\n");
    return 0;
}
