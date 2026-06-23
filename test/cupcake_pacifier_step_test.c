/*
 * TASK-32 Pacifier spawn cycle — run: make test-pacifier-step
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

static void test_pacifier_start_resets(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.pacifier.visible = 1;
    p.pacifier.index = 2;
    p.pacifier.loop = 3;
    p.pacifier.counter = 99;

    cupcake_pacifier_start(&p);

    if (p.pacifier.visible || p.pacifier.index != 0 || p.pacifier.loop != 1 || p.pacifier.counter != 0)
        fail("pacifier start: resets visible/index/loop/counter");
}

static void test_step_increments_counter(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.pacifier.next = 50;
    cupcake_pacifier_step(&p);
    if (p.pacifier.counter != 1)
        fail("pacifier step: increments counter");
}

static void test_spawn_sequence(void)
{
    cupcake_play_state_t p;

    cupcake_set_callback(test_cb);
    memset(&p, 0, sizeof p);
    p.pacifier.next = 1;
    p.pacifier.counter = 0;
    p.pacifier.loop = 1;
    reset_sfx();

    cupcake_pacifier_step(&p);
    if (!p.pacifier.visible || p.pacifier.index != 1 || p.pacifier.loop != 2)
        fail("loop 1: pacifier1 visible, loop advances");
    if (!last_sfx_is("pacifier1"))
        fail("loop 1: pacifier1 SFX");

    cupcake_pacifier_step(&p);
    if (p.pacifier.index != 2 || p.pacifier.loop != 3)
        fail("loop 2: pacifier2 index, loop 3");

    cupcake_pacifier_step(&p);
    if (p.pacifier.visible || p.pacifier.loop != 1 || p.pacifier.counter != 0)
        fail("loop 3 off lane: restart via start()");
}

static void test_loop3_auto_catch_at_lane2(void)
{
    cupcake_play_state_t *p;

    cupcake_set_callback(test_cb);
    cupcake_init();
    p = cupcake_play_state();
    p->pacifier.next = 1;
    p->pacifier.loop = 3;
    p->pacifier.counter = 0;
    p->pacifier.visible = 1;
    p->pacifier.index = 2;
    p->bart.pos = 2;

    cupcake_pacifier_step(p);
    if (p->points != 300u)
        fail("loop 3 lane 2: auto catch +300");
    if (p->pacifier.visible)
        fail("loop 3 lane 2: pacifier restarted hidden");
}

static void test_move_catch_pacifier2(void)
{
    cupcake_play_state_t *p;

    cupcake_set_callback(test_cb);
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();

    p->pacifier.visible = 1;
    p->pacifier.index = 2;
    cupcake_bart_set_position(p, 2);
    cupcake_on_move(CUPCAKE_MOVE_LEFT);
    if (p->points != 300u)
        fail("move: pacifier2 catch at lane 2");
}

int main(void)
{
    test_pacifier_start_resets();
    test_step_increments_counter();
    test_spawn_sequence();
    test_loop3_auto_catch_at_lane2();
    test_move_catch_pacifier2();

    if (g_fail)
        return 1;
    printf("cupcake_pacifier_step: all tests passed\n");
    return 0;
}
