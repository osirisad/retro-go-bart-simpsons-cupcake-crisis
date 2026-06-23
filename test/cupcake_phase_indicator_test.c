/*
 * TASK-54 phase indicator on scoreboard — run: make test-phase-indicator
 */
#include "cupcake.h"
#include "cupcake_scoreboard.h"

#include <stdio.h>
#include <string.h>

static int g_fail;
static char g_sprites[64][16];
static int g_sprite_count;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void capture_sprite(const char *name, void *ctx)
{
    (void)ctx;
    if (!name || g_sprite_count >= 64)
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

static void setup_play(void)
{
    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_start(1, 0);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
}

static void test_start_intro_sets_scoreboard_phase(void)
{
    cupcake_play_state_t *p;

    cupcake_init();
    p = cupcake_play_state();
    p->level = 1;
    cupcake_on_start(3, 0);
    if (p->mode != CUPCAKE_MODE_START)
        fail("start: mode START");
    if (p->scoreboard.phase != 3)
        fail("start: scoreboard.phase set immediately");
    if (p->scoreboard.level != 0)
        fail("start: level overlay cleared for phase display");
}

static void test_play_draws_phase_overlay(void)
{
    cupcake_play_state_t play;

    setup_play();
    memset(&play, 0, sizeof play);
    play = *cupcake_play_state();
    play.mode = CUPCAKE_MODE_PLAY;
    play.scoreboard.phase = 1;
    play.scoreboard.level = 0;

    g_sprite_count = 0;
    cupcake_scoreboard_draw(&play, capture_sprite, NULL);
    if (!has_sprite("digit40") || !has_sprite("digit42"))
        fail("play: P-1 overlay drawn");
}

static void test_start_mode_draws_phase_overlay(void)
{
    cupcake_play_state_t play;

    cupcake_init();
    cupcake_play_state()->level = 1;
    cupcake_on_start(2, 0);

    play = *cupcake_play_state();
    g_sprite_count = 0;
    cupcake_scoreboard_draw(&play, capture_sprite, NULL);
    if (!has_sprite("digit40") || !has_sprite("digit42"))
        fail("start mode: P-2 overlay drawn");
}

static void test_phase_complete_updates_scoreboard_phase(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    setup_play();
    p = cupcake_play_state();
    tm = cupcake_timers();
    p->phase = 1;
    cupcake_scoreboard_set_phase(p, 1);
    cupcake_set_threshold(1000);
    cupcake_add_points(1000);

    if (!cupcake_timer_active(tm, CUPCAKE_TMR_PHASE))
        fail("phase complete: timer started");

    cupcake_timers_update(tm, 4.f);
    if (p->phase != 2)
        fail("phase complete: play phase incremented");
    if (p->scoreboard.phase != 2)
        fail("phase complete: scoreboard.phase updated on restart");
}

static void test_set_phase_clamps(void)
{
    cupcake_play_state_t play;

    memset(&play, 0, sizeof play);
    cupcake_scoreboard_set_phase(&play, 0);
    if (play.scoreboard.phase != 1)
        fail("set_phase: clamp low to 1");
    cupcake_scoreboard_set_phase(&play, 9);
    if (play.scoreboard.phase != CUPCAKE_PHASE_MAX)
        fail("set_phase: clamp high to 6");
}

static void test_con_continue_preserves_phase_on_scoreboard(void)
{
    cupcake_play_state_t *p;

    setup_play();
    p = cupcake_play_state();
    p->phase = 4;
    p->mode = CUPCAKE_MODE_OVER;
    p->scoreboard.show_con = 1;
    cupcake_on_start((int)p->phase, 0);
    if (p->scoreboard.phase != 4)
        fail("CON continue: scoreboard shows resumed phase");
}

int main(void)
{
    test_start_intro_sets_scoreboard_phase();
    test_play_draws_phase_overlay();
    test_start_mode_draws_phase_overlay();
    test_phase_complete_updates_scoreboard_phase();
    test_set_phase_clamps();
    test_con_continue_preserves_phase_on_scoreboard();

    if (g_fail)
        return 1;
    printf("cupcake_phase_indicator: all tests passed\n");
    return 0;
}
