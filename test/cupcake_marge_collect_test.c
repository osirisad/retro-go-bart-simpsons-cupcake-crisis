/*
 * TASK-29 Marge collect delivery bonus — run: make test-marge-collect
 */
#include "cupcake.h"

#include <stdio.h>
#include <string.h>

static int g_fail;
static char g_sfx[32][16];
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
    if (type == CUPCAKE_CB_SFX && str && g_sfx_count < 32) {
        snprintf(g_sfx[g_sfx_count], sizeof g_sfx[0], "%s", str);
        g_sfx_count++;
    }
    return 0;
}

static int sfx_count(const char *id)
{
    int i, n = 0;

    for (i = 0; i < g_sfx_count; i++)
        if (strcmp(g_sfx[i], id) == 0)
            n++;
    return n;
}

static void reset_sfx(void)
{
    g_sfx_count = 0;
}

static void test_collect_guards(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    if (cupcake_marge_collect(&p))
        fail("collect: false when marge hidden");

    p.marge.visible = 1;
    p.bart.pos = 1;
    p.bart.count = 2;
    if (cupcake_marge_collect(&p))
        fail("collect: false when not at position 0");
}

static void test_normal_delivery(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    uint32_t before;

    cupcake_set_callback(test_cb);
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();
    tm = cupcake_timers();
    before = p->points;
    reset_sfx();

    p->marge.visible = 1;
    p->bart.count = 3;
    cupcake_bart_set_position(p, 0);

    if (!cupcake_marge_collect(p))
        fail("normal: collect returns true");
    if (sfx_count("deliver") != 1)
        fail("normal: deliver SFX");
    if (!p->scoreboard.bonus_active)
        fail("normal: bonus active");
    if (!cupcake_is_paused())
        fail("normal: paused during bonus");

    cupcake_timers_update(tm, 0.06f);
    cupcake_timers_update(tm, 0.06f);
    cupcake_timers_update(tm, 0.06f);

    if (p->points != before + 300u)
        fail("normal: +300 points for 3 cupcakes");
    if (sfx_count("points") != 3)
        fail("normal: points SFX each tick");
    if (p->scoreboard.bonus_active)
        fail("normal: bonus finished");

    cupcake_timers_update(tm, 0.5f * cupcake_game_tick_rate_sec(p));
    if (p->marge.visible)
        fail("normal: marge.start after scheduled delay");
}

static void test_five_cupcake_delivery(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    cupcake_set_callback(test_cb);
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();
    tm = cupcake_timers();
    reset_sfx();

    p->marge.visible = 1;
    p->bart.count = 5;
    cupcake_bart_set_position(p, 0);

    if (!cupcake_marge_collect(p))
        fail("five: collect returns true");
    if (sfx_count("deliver") != 0)
        fail("five: no deliver SFX");
    if (p->scoreboard.bonus_points_left != 10)
        fail("five: 10 bonus ticks (count+5)");

    cupcake_timers_update(tm, 0.025f);
    if (sfx_count("five") != 1)
        fail("five: five SFX on first tick");
    if (sfx_count("points") != 0)
        fail("five: no points SFX during five delivery");

    cupcake_timers_update(tm, 0.025f);
    cupcake_timers_update(tm, 0.025f);
    cupcake_timers_update(tm, 0.025f);
    cupcake_timers_update(tm, 0.025f);
    /* tick 5: giveCupcakes + marge.start */
    if (p->bart.count != 0)
        fail("five: giveCupcakes clears held stack at tick 5");
    if (p->marge.visible)
        fail("five: marge.start at tick 5");

    cupcake_timers_update(tm, 0.025f);
    cupcake_timers_update(tm, 0.025f);
    cupcake_timers_update(tm, 0.025f);
    cupcake_timers_update(tm, 0.025f);
    cupcake_timers_update(tm, 0.025f);
    if (p->points != 1000u)
        fail("five: 10 ticks * 100 points");
}

int main(void)
{
    test_collect_guards();
    test_normal_delivery();
    test_five_cupcake_delivery();

    if (g_fail)
        return 1;
    printf("cupcake_marge_collect: all tests passed\n");
    return 0;
}
