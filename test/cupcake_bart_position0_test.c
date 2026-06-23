/*
 * TASK-49 position 0 hand-off layers — run: make test-bart-position0
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

static void test_position0_handoff_layers(void)
{
    cupcake_play_state_t p;
    const int handoff[] = {0, 1};
    const uint16_t mask = cupcake_bart_position_layers(0);

    memset(&p, 0, sizeof p);
    cupcake_bart_set_position(&p, 0);
    expect_only_materials(&p, handoff, 2, "pos0 hand-off");
    if (mask != p.bart.visible)
        fail("pos0: layer mask matches visible state");
    if (cupcake_bart_material_visible(&p, 7))
        fail("pos0: no bart7 (lane-1 arm layer)");
}

static void test_position1_distinct_from_zero(void)
{
    cupcake_play_state_t p;
    const int lane1[] = {1, 7};

    memset(&p, 0, sizeof p);
    cupcake_bart_set_position(&p, 1);
    expect_only_materials(&p, lane1, 2, "pos1");
    if (cupcake_bart_material_visible(&p, 0))
        fail("pos1: no bart0");
}

static void test_action_throw_uses_handoff_pose(void)
{
    cupcake_play_state_t *p;
    const int handoff[] = {0, 1};

    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();
    cupcake_bart_set_position(p, 1);

    cupcake_set_buttons(CUPCAKE_BTN_ACTION);
    cupcake_update();
    cupcake_set_buttons(0);

    expect_only_materials(p, handoff, 2, "action onStart pos0");
}

static void test_lane1_to_handoff_clears_bart7(void)
{
    cupcake_play_state_t p;
    const int handoff[] = {0, 1};

    memset(&p, 0, sizeof p);
    cupcake_bart_set_position(&p, 1);
    cupcake_bart_set_position(&p, 0);
    expect_only_materials(&p, handoff, 2, "1->0 transition");
}

int main(void)
{
    test_position0_handoff_layers();
    test_position1_distinct_from_zero();
    test_action_throw_uses_handoff_pose();
    test_lane1_to_handoff_clears_bart7();

    if (g_fail)
        return 1;
    printf("cupcake_bart_position0: all tests passed\n");
    return 0;
}
