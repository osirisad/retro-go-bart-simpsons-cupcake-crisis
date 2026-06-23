/*
 * TASK-14 phase complete and restart flow — run: make test-phase-complete
 */
#include "cupcake.h"

#include <stdio.h>
#include <string.h>

static int g_fail;
static char g_sfx[16][16];
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
    if (type == CUPCAKE_CB_SFX && str && g_sfx_count < 16) {
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

static void test_phase_complete_pauses_and_runs_timer(void)
{
    cupcake_timers_t *tm;

    setup_play();
    tm = cupcake_timers();
    cupcake_set_threshold(1000);
    cupcake_add_points(1000);

    if (!cupcake_is_paused())
        fail("phase complete: game paused");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("phase complete: 4s phase timer active");
    if (sfx_count("stop") < 1 || sfx_count("phase") < 1)
        fail("phase complete: stop + phase SFX on timer start");
}

static void test_phase_complete_end_increments_and_restarts(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    setup_play();
    p = cupcake_play_state();
    tm = cupcake_timers();
    p->phase = 1;
    p->miss.count = 2;
    cupcake_set_threshold(1000);
    cupcake_add_points(1000);

    cupcake_timers_update(tm, 4.f);
    if (p->phase != 2)
        fail("phase complete end: phase incremented");
    if (p->miss.count != 1)
        fail("phase complete end: miss decreased");
    if (p->scoreboard.phase != 2)
        fail("phase restart: scoreboard phase updated");
    if (!cupcake_is_paused())
        fail("phase restart: paused during 0.75s interstitial");

    cupcake_timers_update(tm, 0.75f);
    if (cupcake_is_paused())
        fail("phase restart: resumed after interstitial");
    if (!p->enabled)
        fail("phase restart: enabled restored");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_GAME))
        fail("phase restart: game timer restarted");
}

static void test_phase_complete_clamps_at_max(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    setup_play();
    p = cupcake_play_state();
    tm = cupcake_timers();
    p->phase = CUPCAKE_PHASE_MAX;
    cupcake_set_threshold(1000);
    cupcake_add_points(1000);

    cupcake_timers_update(tm, 4.f);
    if (p->phase != CUPCAKE_PHASE_MAX)
        fail("phase complete: clamped at phase max");
}

static void test_phase_restart_resets_entities(void)
{
    cupcake_play_state_t *p;

    setup_play();
    p = cupcake_play_state();
    cupcake_grid_set_visible(&p->grid, 2, 3, 1);
    cupcake_on_phase_restart();
    if (p->grid.visible != 0)
        fail("phase restart: cupcakes grid cleared");
    if (p->bart.pos != 2 || p->bart.count != 0)
        fail("phase restart: bart reset to lane 2");
}

static void test_miss_restart_skips_phase_timer(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    setup_play();
    p = cupcake_play_state();
    tm = cupcake_timers();
    p->phase = 3;
    p->miss.count = 1;
    g_sfx_count = 0;

    cupcake_miss_increase();
    if (cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("miss restart: no phase-complete timer");
    if (p->phase != 3)
        fail("miss restart: phase unchanged");
    if (p->scoreboard.phase != 3)
        fail("miss restart: scoreboard phase synced");
}

int main(void)
{
    test_phase_complete_pauses_and_runs_timer();
    test_phase_complete_end_increments_and_restarts();
    test_phase_complete_clamps_at_max();
    test_phase_restart_resets_entities();
    test_miss_restart_skips_phase_timer();

    if (g_fail)
        return 1;
    printf("cupcake_phase_complete: all tests passed\n");
    return 0;
}
