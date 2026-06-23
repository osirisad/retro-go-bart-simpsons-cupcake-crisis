/*
 * TASK-17 Bart move — run: make test-bart-move
 */
#include "cupcake.h"

#include <stdio.h>
#include <string.h>

static int g_fail;
static char g_sfx[12][16];
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
    if (type == CUPCAKE_CB_SFX && str && g_sfx_count < 12) {
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

static void enter_play(cupcake_play_state_t **out)
{
    cupcake_set_callback(test_cb);
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    g_sfx_count = 0;
    *out = cupcake_play_state();
}

static void test_normal_move(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (p->bart.pos != 3)
        fail("move right: lane 3");
    if (!last_sfx_is("move"))
        fail("move right: move SFX");

    g_sfx_count = 0;
    cupcake_on_move(CUPCAKE_MOVE_LEFT);
    if (p->bart.pos != 2)
        fail("move left: lane 2");
    if (!last_sfx_is("move"))
        fail("move left: move SFX");
}

static void test_lane_bounds(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    cupcake_on_move(CUPCAKE_MOVE_LEFT);
    if (p->bart.pos != 1)
        fail("move left: lane 2 -> 1");
    cupcake_on_move(CUPCAKE_MOVE_LEFT);
    if (p->bart.pos != 1)
        fail("move left at lane 1: no change");
    cupcake_bart_set_position(p, 4);
    cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (p->bart.pos != 4)
        fail("move right at lane 4: no change");
}

static void test_catch_cupcake(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    cupcake_grid_set_visible(&p->grid, 3, 1, 1);
    cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (p->bart.pos != 3)
        fail("catch: moved to lane 3");
    if (p->bart.count != 1)
        fail("catch: count incremented");
    if (!cupcake_grid_is_visible(&p->grid, 3, 1))
        fail("catch: held stack slot 1 shown on lane");
    if (p->points != 100u)
        fail("catch: +100 points");
    if (!last_sfx_is("cupcake"))
        fail("catch: cupcake SFX");
}

static void test_full_stack_triggers_miss(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    p->bart.count = 5;
    cupcake_bart_set_position(p, 2);
    cupcake_grid_set_visible(&p->grid, 3, 1, 1);
    cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (p->bart.pos != 3)
        fail("full stack: moved to lane 3");
    if (!cupcake_is_paused())
        fail("full stack: onM_Cupcake pauses");
    if (!cupcake_timer_active(cupcake_timers(), CUPCAKE_TMR_MISS))
        fail("full stack: miss timer started");
}

static void test_catch_pacifier(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    p->pacifier.visible = 1;
    p->pacifier.index = 2;
    cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (p->points != 300u)
        fail("pacifier: +300 points");
    if (!last_sfx_is("move"))
        fail("pacifier: move still completes after catch");
    if (p->pacifier.visible)
        fail("pacifier: restarted hidden");
}

static void test_marge_collect_on_move(void)
{
    cupcake_play_state_t *p;
    int slot;

    enter_play(&p);
    p->marge.visible = 1;
    p->bart.count = 3;
    cupcake_bart_set_position(p, 0);
    for (slot = 1; slot <= CUPCAKE_GRID_SLOTS; slot++)
        cupcake_grid_set_visible(&p->grid, 1, slot, 0);
    cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (p->bart.count != 0)
        fail("marge collect: count cleared");
    if (p->bart.pos != 1)
        fail("marge collect: moved to lane 1");
}

static void test_marge_collect_keeps_one_cupcake(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    p->marge.visible = 1;
    p->bart.count = 3;
    cupcake_bart_set_position(p, 0);
    cupcake_grid_set_visible(&p->grid, 1, 1, 1);
    cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (p->bart.count != 2)
        fail("marge collect + catch: count 1 then catch to 2");
}

int main(void)
{
    test_normal_move();
    test_lane_bounds();
    test_catch_cupcake();
    test_full_stack_triggers_miss();
    test_catch_pacifier();
    test_marge_collect_on_move();
    test_marge_collect_keeps_one_cupcake();

    if (g_fail)
        return 1;
    printf("cupcake_bart_move: all tests passed\n");
    return 0;
}
