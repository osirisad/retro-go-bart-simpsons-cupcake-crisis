/*
 * TASK-18 Bart sit on couch — run: make test-bart-sit
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

static int sfx_count(const char *id)
{
    int i, n = 0;
    for (i = 0; i < g_sfx_count; i++)
        if (strcmp(g_sfx[i], id) == 0)
            n++;
    return n;
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

static void setup_couch(cupcake_play_state_t *p, int frame_index)
{
    p->couch.onscreen = 1;
    p->couch.frame_visible = 0;
    cupcake_couch_set_frame_visible(&p->couch, frame_index, 1);
    cupcake_bart_set_position(p, 4);
}

static void test_sit_requires_lane4_and_couch(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    cupcake_on_move(CUPCAKE_MOVE_UP);
    if (p->bart.pos != 2)
        fail("sit: ignored without couch at lane 2");

    setup_couch(p, 3);
    p->bart.pos = 3;
    cupcake_on_move(CUPCAKE_MOVE_UP);
    if (p->bart.pos != 3)
        fail("sit: ignored when not lane 4");
}

static void test_sit_moves_to_lane5_and_pauses(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    enter_play(&p);
    tm = cupcake_timers();
    setup_couch(p, 3);
    cupcake_on_move(CUPCAKE_MOVE_DOWN);
    if (p->bart.pos != 5)
        fail("sit: position 5");
    if (!cupcake_is_paused())
        fail("sit: game paused");
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_ACTION))
        fail("sit: 2s bonus timer started");
}

static void test_sit_bonus_sfx(void)
{
    cupcake_play_state_t *p;

    enter_play(&p);
    setup_couch(p, 2);
    cupcake_on_move(CUPCAKE_MOVE_UP);
    if (sfx_count("bonus") < 1)
        fail("sit: bonus SFX on timer start");
}

static void run_sit_bonus(cupcake_timers_t *tm, cupcake_play_state_t *p, uint32_t *points_out)
{
    cupcake_timers_update(tm, 2.f);
    while (p->scoreboard.bonus_active)
        cupcake_timers_update(tm, 0.03f);
    *points_out = p->points;
}

static void test_sit_bonus_points_by_frame(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;
    uint32_t pts;

    enter_play(&p);
    tm = cupcake_timers();
    setup_couch(p, 3);
    cupcake_on_move(CUPCAKE_MOVE_UP);
    run_sit_bonus(tm, p, &pts);
    if (pts != 100u)
        fail("sit: couch3 awards 100");

    enter_play(&p);
    tm = cupcake_timers();
    setup_couch(p, 2);
    cupcake_on_move(CUPCAKE_MOVE_UP);
    run_sit_bonus(tm, p, &pts);
    if (pts != 200u)
        fail("sit: couch2 awards 200");

    enter_play(&p);
    tm = cupcake_timers();
    setup_couch(p, 1);
    cupcake_on_move(CUPCAKE_MOVE_UP);
    run_sit_bonus(tm, p, &pts);
    if (pts != 400u)
        fail("sit: couch1 awards 400");
}

static void test_sit_restarts_couch(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    enter_play(&p);
    tm = cupcake_timers();
    setup_couch(p, 3);
    cupcake_on_move(CUPCAKE_MOVE_UP);
    cupcake_timers_update(tm, 2.f);
    if (p->couch.onscreen)
        fail("sit: couch restarted off-screen after bonus timer");
}

static void test_sit_couch4_no_bonus_timer(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    enter_play(&p);
    tm = cupcake_timers();
    setup_couch(p, 4);
    cupcake_on_move(CUPCAKE_MOVE_UP);
    if (p->bart.pos != 5)
        fail("sit couch4: position 5");
    if (cupcake_timer_active(tm, CUPCAKE_TMR_ACTION))
        fail("sit couch4: no bonus timer");
    if (cupcake_is_paused())
        fail("sit couch4: resumed without bonus");
    if (p->couch.onscreen)
        fail("sit couch4: couch restarted off-screen");
}

static void advance_couch_timer(cupcake_timers_t *tm, int ticks)
{
    int i;

    cupcake_timers_update(tm, 0.001f);
    for (i = 0; i < ticks; i++)
        cupcake_timers_update(tm, 3.f);
}

static void test_sit_before_couch4_no_miss(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    enter_play(&p);
    tm = cupcake_timers();
    p->couch.counter = p->couch.next - 1;
    cupcake_couch_step(p, (int)p->game_tick);
    cupcake_couch_set_frame_visible(&p->couch, 2, 1);
    cupcake_bart_set_position(p, 4);
    cupcake_on_move(CUPCAKE_MOVE_UP);

    advance_couch_timer(tm, 4);
    if (cupcake_timer_active(tm, CUPCAKE_TMR_MISS))
        fail("sit before couch4: couch miss not triggered");
}

int main(void)
{
    test_sit_requires_lane4_and_couch();
    test_sit_moves_to_lane5_and_pauses();
    test_sit_bonus_sfx();
    test_sit_bonus_points_by_frame();
    test_sit_restarts_couch();
    test_sit_couch4_no_bonus_timer();
    test_sit_before_couch4_no_miss();

    if (g_fail)
        return 1;
    printf("cupcake_bart_sit: all tests passed\n");
    return 0;
}
