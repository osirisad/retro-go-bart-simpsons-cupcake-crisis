/*
 * TASK-20 Bart catchCupcake / catchPacifier — run: make test-bart-catch
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

static void test_catch_cupcake(void)
{
    cupcake_play_state_t *p;
    int n;

    enter_play(&p);
    cupcake_bart_set_position(p, 3);
    n = cupcake_bart_catch_cupcake(p, 3);
    if (n != 1 || p->bart.count != 1)
        fail("catchCupcake: count incremented");
    if (p->points != 100u)
        fail("catchCupcake: +100 points");
    if (!last_sfx_is("cupcake"))
        fail("catchCupcake: cupcake SFX");
    if (p->bart.pos != 3)
        fail("catchCupcake: position set");

    p->bart.count = 5;
    if (cupcake_bart_catch_cupcake(p, 3) >= 0)
        fail("catchCupcake: max 5 blocks catch");
}

static void test_catch_pacifier(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    p->pacifier.visible = 1;
    p->pacifier.index = 2;
    cupcake_bart_catch_pacifier(p);
    if (p->points != 300u)
        fail("catchPacifier: +300 points");
    if (!last_sfx_is("pacifier2"))
        fail("catchPacifier: pacifier2 SFX");
    if (p->pacifier.visible)
        fail("catchPacifier: pacifier restarted hidden");
}

static void test_aircakes_land_catch(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    cupcake_bart_set_position(p, 2);
    cupcake_aircakes_land_at_lane(p, 2);
    if (p->bart.count != 1 || p->points != 100u)
        fail("aircakes land: catch at bart lane");
    if (!cupcake_grid_is_visible(&p->grid, 2, 1))
        fail("aircakes land: held stack slot 1 shown after catch");
}

static void test_aircakes_land_grid(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    cupcake_bart_set_position(p, 2);
    cupcake_aircakes_land_at_lane(p, 3);
    if (p->bart.count != 0)
        fail("aircakes land: no catch off lane");
    if (!cupcake_grid_is_visible(&p->grid, 3, 1))
        fail("aircakes land: grid cell set off lane");
}

static void test_aircakes_land_miss_when_full(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    p->bart.count = 5;
    cupcake_bart_set_position(p, 2);
    cupcake_aircakes_land_at_lane(p, 2);
    if (!cupcake_is_paused())
        fail("aircakes land full: miss sequence");
}

static void test_pacifier_loop3_catch(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    cupcake_bart_set_position(p, 2);
    p->pacifier.visible = 1;
    p->pacifier.index = 2;
    cupcake_pacifier_on_loop3(p);
    if (p->points != 300u)
        fail("pacifier loop3: catch at lane 2");
}

static void test_move_still_catches(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    cupcake_grid_set_visible(&p->grid, 3, 1, 1);
    cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (p->bart.count != 1 || p->points != 100u)
        fail("move: still uses catchCupcake");
}

int main(void)
{
    test_catch_cupcake();
    test_catch_pacifier();
    test_aircakes_land_catch();
    test_aircakes_land_grid();
    test_aircakes_land_miss_when_full();
    test_pacifier_loop3_catch();
    test_move_still_catches();

    if (g_fail)
        return 1;
    printf("cupcake_bart_catch: all tests passed\n");
    return 0;
}
