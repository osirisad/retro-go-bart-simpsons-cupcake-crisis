/*
 * TASK-16 Bart start and initial spawn state — run: make test-bart-start
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
        fprintf(stderr, "FAIL: %s material bart%d expected %d got %d\n", label, index, want, got);
        g_fail = 1;
    }
}

static void test_bart_start_lane2(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.bart.pos = 4;
    p.bart.count = 3;
    p.bart.visible = (uint16_t)(1u << 4 | 1u << 8);
    cupcake_grid_set_visible(&p.grid, 4, 1, 1);
    cupcake_grid_set_visible(&p.grid, 4, 2, 1);

    cupcake_bart_start(&p, 2);

    if (p.bart.pos != 2)
        fail("bart start: position 2");
    if (p.bart.count != 0)
        fail("bart start: count reset");
    if (p.bart.miss_index != -1)
        fail("bart start: miss pose cleared");

    expect_material(&p, 2, 1, "lane2");
    expect_material(&p, 0, 0, "lane2");
    expect_material(&p, 1, 0, "lane2");
    expect_material(&p, 7, 0, "lane2");
    expect_material(&p, 8, 0, "lane2");

    if (cupcake_grid_is_visible(&p.grid, 2, 1))
        fail("bart start: held stack hidden at count 0");
    if (cupcake_grid_is_visible(&p.grid, 4, 1))
        fail("bart start: prior lane stack cleared");
}

static void test_position_setter_layers(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);

    cupcake_bart_set_position(&p, 0);
    expect_material(&p, 0, 1, "pos0");
    expect_material(&p, 1, 1, "pos0");

    cupcake_bart_set_position(&p, 1);
    expect_material(&p, 1, 1, "pos1");
    expect_material(&p, 7, 1, "pos1");
    expect_material(&p, 0, 0, "pos1");

    cupcake_bart_set_position(&p, 4);
    expect_material(&p, 4, 1, "pos4");
    expect_material(&p, 8, 1, "pos4");

    cupcake_bart_set_position(&p, 5);
    expect_material(&p, 5, 1, "pos5");
    expect_material(&p, 8, 1, "pos5");
}

static void test_start_sequence_and_phase_start(void)
{
    cupcake_play_state_t *p;

    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_start(1, 0);
    cupcake_timers_update(cupcake_timers(), 3.16f);

    p = cupcake_play_state();
    if (p->bart.pos != 2 || p->bart.count != 0)
        fail("start onStart: bart reset");
    if (!cupcake_bart_material_visible(p, 2))
        fail("start onStart: bart2 visible");

    cupcake_timers_update(cupcake_timers(), 3.16f);
    if (p->bart.pos != 2 || p->bart.count != 0)
        fail("phase start: bart lane 2 count 0");
    if (!cupcake_bart_material_visible(p, 2))
        fail("phase start: bart2 visible");
    for (int i = 0; i < 10; i++) {
        if (i == 2)
            continue;
        if (cupcake_bart_material_visible(p, i))
            fail("phase start: extra bart material visible");
    }
}

int main(void)
{
    test_bart_start_lane2();
    test_position_setter_layers();
    test_start_sequence_and_phase_start();

    if (g_fail)
        return 1;
    printf("cupcake_bart_start: all tests passed\n");
    return 0;
}
