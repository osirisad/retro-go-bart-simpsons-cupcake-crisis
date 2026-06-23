/*
 * TASK-33 Miss counter ($P.M_) — run: make test-miss-counter
 */
#include "cupcake.h"

#include <stdio.h>
#include <string.h>

static int g_fail;
static char g_sprites[8][16];
static int g_sprite_count;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void capture_sprite(const char *name, void *ctx)
{
    (void)ctx;
    if (!name || g_sprite_count >= 8)
        return;
    snprintf(g_sprites[g_sprite_count], sizeof g_sprites[0], "%s", name);
    g_sprite_count++;
}

static int has_sprite(const char *name)
{
    int i;

    for (i = 0; i < g_sprite_count; i++)
        if (strcmp(g_sprites[i], name) == 0)
            return 1;
    return 0;
}

static void test_miss_start_clears(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.miss.count = 2;
    cupcake_miss_start(&p, 0);
    if (p.miss.count != 0)
        fail("miss start(0): clears count");
}

static void test_miss_start_with_initial(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_miss_start(&p, 2);

    g_sprite_count = 0;
    cupcake_miss_draw_visible(&p, capture_sprite, NULL);
    if (p.miss.count != 2)
        fail("miss start(2): count");
    if (!has_sprite("miss1") || !has_sprite("miss2") || has_sprite("miss3"))
        fail("miss start(2): draws miss1 and miss2");
}

static void test_miss_increase_via_on_m(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();
    tm = cupcake_timers();

    cupcake_on_m_cupcake(1);
    cupcake_timers_update(tm, 0.5f);
    cupcake_timers_update(tm, 1.75f);
    if (p->miss.count != 1)
        fail("miss increase via onM_: count 1");
}

static void test_miss_decrease_draw(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.miss.count = 2;
    cupcake_miss_decrease(&p);
    if (p.miss.count != 1)
        fail("miss decrease: count 1");

    g_sprite_count = 0;
    cupcake_miss_draw_visible(&p, capture_sprite, NULL);
    if (!has_sprite("miss1") || has_sprite("miss2"))
        fail("miss decrease: only miss1 drawn");
}

static void test_phase_complete_decreases_miss(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();
    tm = cupcake_timers();

    p->miss.count = 2;
    cupcake_on_phase_complete();
    cupcake_timers_update(tm, 4.f);
    if (p->miss.count != 1)
        fail("phase complete: miss decreased");
}

static void test_game_over_shows_three_miss_icons(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.mode = CUPCAKE_MODE_OVER;
    p.miss.count = 3;

    g_sprite_count = 0;
    cupcake_miss_draw_visible(&p, capture_sprite, NULL);
    if (!has_sprite("miss1") || !has_sprite("miss2") || !has_sprite("miss3"))
        fail("over mode: all three miss icons");
}

static void test_miss_couch_stops_couch_timer(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_quick_start(1);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    p = cupcake_play_state();
    tm = cupcake_timers();

    p->couch.counter = p->couch.next - 1;
    cupcake_couch_step(p, (int)p->game_tick);
    if (!cupcake_timer_active(tm, CUPCAKE_TMR_COUCH))
        fail("miss couch setup: couch timer running");

    cupcake_on_m_couch();
    if (cupcake_timer_active(tm, CUPCAKE_TMR_COUCH))
        fail("miss couch: couch spawn timer stopped");
    if (p->marge.visible || p->pacifier.visible)
        fail("miss couch: marge/pacifier hidden during miss");
}

int main(void)
{
    test_miss_start_clears();
    test_miss_start_with_initial();
    test_miss_increase_via_on_m();
    test_miss_decrease_draw();
    test_phase_complete_decreases_miss();
    test_game_over_shows_three_miss_icons();
    test_miss_couch_stops_couch_timer();

    if (g_fail)
        return 1;
    printf("cupcake_miss_counter: all tests passed\n");
    return 0;
}
