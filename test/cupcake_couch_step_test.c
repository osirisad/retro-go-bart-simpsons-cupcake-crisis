/*
 * TASK-30 Couch spawn timer and animation — run: make test-couch-step
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

static void reset_sfx(void)
{
    g_sfx_count = 0;
}

static void test_couch_start_resets(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.couch.onscreen = 1;
    p.couch.counter = 50;
    p.couch.frame_visible = 0x1fu;

    cupcake_couch_start(&p);

    if (p.couch.onscreen || p.couch.counter != 0 || p.couch.frame_visible != 0)
        fail("couch start: resets onscreen/counter/frames");
}

static void test_couch_step_increments_counter(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.couch.next = 99;
    cupcake_couch_step(&p, 1);
    if (p.couch.counter != 1)
        fail("couch step: increments counter");
    cupcake_couch_step(&p, 2);
    if (p.couch.counter != 2)
        fail("couch step: counter continues");
}

static void test_couch_step_no_increment_when_onscreen(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.couch.onscreen = 1;
    p.couch.counter = 5;
    cupcake_couch_step(&p, 1);
    if (p.couch.counter != 5)
        fail("couch step: idle while onscreen");
}

static void test_spawn_when_counter_reaches_next(void)
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

    p->couch.counter = p->couch.next - 1;
    p->marge.visible = 0;
    cupcake_couch_step(p, (int)p->game_tick);

    if (!p->couch.onscreen)
        fail("spawn: onscreen after counter >= next");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_COUCH))
        fail("spawn: couch animation timer started");
    if (!(p->couch.frame_visible & (1u << 0)))
        fail("spawn: couch0 visible on timer start");
}

static void test_marge_blocks_spawn(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.couch.next = 1;
    p.couch.counter = 1;
    p.marge.visible = 1;
    cupcake_couch_step(&p, 10);
    if (p.couch.onscreen)
        fail("spawn: blocked while marge visible");
}

static void advance_couch_timer(cupcake_timers_t *tm, int ticks)
{
    int i;

    /* Couch timer started from game OT sets skip_dt_once; prime it before 3s steps. */
    cupcake_timers_update(tm, 0.001f);
    for (i = 0; i < ticks; i++)
        cupcake_timers_update(tm, 3.f);
}

static void test_spawn_animation_frames_and_sfx(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    cupcake_set_callback(test_cb);
    reset_sfx();
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();
    tm = cupcake_timers();

    p->couch.counter = p->couch.next - 1;
    cupcake_couch_step(p, (int)p->game_tick);
    reset_sfx();

    advance_couch_timer(tm, 1);
    if (!(p->couch.frame_visible & (1u << 1)))
        fail("anim: couch1 after tick 1");
    if (sfx_count("couch") < 1)
        fail("anim: couch SFX on OT");

    advance_couch_timer(tm, 1);
    if (!(p->couch.frame_visible & (1u << 2)))
        fail("anim: couch2 after tick 2");

    advance_couch_timer(tm, 1);
    if (!(p->couch.frame_visible & (1u << 3)))
        fail("anim: couch3 after tick 3");
    if (p->maggie.visible)
        fail("anim: couch3 hides maggie");

    advance_couch_timer(tm, 1);
    if (!(p->couch.frame_visible & (1u << 4)))
        fail("anim: couch4 after tick 4");
}

static void test_spawn_end_triggers_miss(void)
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

    cupcake_bart_set_position(p, 2);
    p->couch.counter = p->couch.next - 1;
    cupcake_couch_step(p, (int)p->game_tick);
    advance_couch_timer(tm, 4);

    if (!cupcake_timer_active(tm, CUPCAKE_TMR_MISS))
        fail("spawn end: onM_Couch miss timer when Bart not sitting");
    if (!cupcake_is_paused())
        fail("spawn end: game paused for miss");
}

static void test_spawn_end_skips_miss_when_sitting(void)
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

    p->couch.counter = p->couch.next - 1;
    cupcake_couch_step(p, (int)p->game_tick);
    cupcake_couch_set_frame_visible(&p->couch, 3, 1);
    cupcake_bart_set_position(p, 4);
    cupcake_on_move(CUPCAKE_MOVE_UP);
    if (p->bart.pos != 5)
        fail("sit setup: Bart at lane 5");

    advance_couch_timer(tm, 4);
    if (cupcake_timer_active(tm, CUPCAKE_TMR_MISS))
        fail("spawn end: no miss while Bart sitting");
}

int main(void)
{
    test_couch_start_resets();
    test_couch_step_increments_counter();
    test_couch_step_no_increment_when_onscreen();
    test_spawn_when_counter_reaches_next();
    test_marge_blocks_spawn();
    test_spawn_animation_frames_and_sfx();
    test_spawn_end_triggers_miss();
    test_spawn_end_skips_miss_when_sitting();

    if (g_fail)
        return 1;
    printf("cupcake_couch_step: all tests passed\n");
    return 0;
}
