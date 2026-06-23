/*
 * TASK-11 animated addBonus counter — run: make test-add-bonus
 */
#include "cupcake.h"
#include "cupcake_scoreboard.h"

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

static void setup_play(void)
{
    cupcake_set_callback(test_cb);
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_start(1, 0);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    g_sfx_count = 0;
}

static void test_couch_style_bonus_ticks_points(void)
{
    cupcake_timers_t *tm = cupcake_timers();
    cupcake_play_state_t *p = cupcake_play_state();
    uint32_t before = p->points;

    setup_play();
    cupcake_scoreboard_add_bonus(0.03f, 3, 100);

    if (!p->scoreboard.bonus_active)
        fail("couch bonus: active after start");
    if (!cupcake_is_paused())
        fail("couch bonus: game paused during ticks");

    cupcake_timers_update(tm, 0.03f);
    if (p->points != before + 100u)
        fail("couch bonus: tick1 +100 points");
    if (p->scoreboard.score != p->points)
        fail("couch bonus: scoreboard synced on tick1");
    if (sfx_count("points") != 1)
        fail("couch bonus: points SFX each tick");

    cupcake_timers_update(tm, 0.03f);
    cupcake_timers_update(tm, 0.03f);
    if (p->points != before + 300u)
        fail("couch bonus: three ticks total +300");
    if (p->scoreboard.bonus_active)
        fail("couch bonus: finished after 3 ticks");
    if (cupcake_is_paused())
        fail("couch bonus: resumed after onEnd");
}

static void test_five_cupcake_first_tick_plays_five(void)
{
    cupcake_timers_t *tm = cupcake_timers();
    cupcake_scoreboard_bonus_cfg_t cfg;

    setup_play();
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 0.025f;
    cfg.point_ticks = 2;
    cfg.increment = 100;
    cfg.play_five_on_first = 1;
    cfg.play_points_sfx = 0;

    cupcake_scoreboard_add_bonus_ex(cupcake_play_state(), tm, &cfg);
    cupcake_timers_update(tm, 0.025f);
    if (sfx_count("five") != 1)
        fail("five delivery: first tick plays five");
    if (sfx_count("points") != 0)
        fail("five delivery: no points SFX when play_points_sfx=0");

    cupcake_timers_update(tm, 0.025f);
    if (cupcake_play_state()->points != 200u)
        fail("five delivery: two ticks add 200 points");
}

static void test_on_score_change_triggers_phase_complete(void)
{
    cupcake_timers_t *tm = cupcake_timers();
    cupcake_play_state_t *p = cupcake_play_state();

    setup_play();
    p->phase = 1;
    p->phase_threshold = 250;
    p->points = 0;
    p->scoreboard.score = 0;

    cupcake_scoreboard_add_bonus(0.03f, 3, 100);
    cupcake_timers_update(tm, 0.03f);
    cupcake_timers_update(tm, 0.03f);
    cupcake_timers_update(tm, 0.03f);

    if (p->points != 300u)
        fail("onScoreChange path: points accumulated");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("onScoreChange path: phase complete timer started at threshold");
}

static int g_bonus_ended;

static void bonus_test_on_end(void *ctx)
{
    (void)ctx;
    g_bonus_ended = 1;
}

static void test_custom_on_end(void)
{
    cupcake_timers_t *tm = cupcake_timers();
    cupcake_scoreboard_bonus_cfg_t cfg;

    setup_play();
    g_bonus_ended = 0;
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 0.03f;
    cfg.point_ticks = 1;
    cfg.increment = 50;
    cfg.play_points_sfx = 0;
    cfg.on_end = bonus_test_on_end;

    cupcake_scoreboard_add_bonus_ex(cupcake_play_state(), tm, &cfg);
    cupcake_timers_update(tm, 0.03f);
    if (!g_bonus_ended)
        fail("addBonus: custom onEnd fired");
}

int main(void)
{
    test_couch_style_bonus_ticks_points();
    test_five_cupcake_first_tick_plays_five();
    test_on_score_change_triggers_phase_complete();
    test_custom_on_end();

    if (g_fail)
        return 1;
    printf("cupcake_add_bonus: all tests passed\n");
    return 0;
}
