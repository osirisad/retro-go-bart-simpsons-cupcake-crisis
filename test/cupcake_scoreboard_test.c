/*
 * TASK-10 scoreboard digit renderer — run: make test-scoreboard
 */
#include "cupcake.h"
#include "cupcake_scoreboard.h"
#include "cupcake_state.h"

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

static void reset_sprites(void)
{
    g_sprite_count = 0;
}

static void test_draw_digits_full_score(void)
{
    reset_sprites();
    cupcake_scoreboard_draw_digits(12345, 2, 5, capture_sprite, NULL);
    if (!has_sprite("digit00") || !has_sprite("digit02"))
        fail("draw_digits: digit 5 on ones column");
    if (!has_sprite("digit11") || !has_sprite("digit13"))
        fail("draw_digits: digit 4 on tens column");
    if (!has_sprite("digit30") || !has_sprite("digit32"))
        fail("draw_digits: digit 3 on hundreds column");
    if (!has_sprite("digit41"))
        fail("draw_digits: ten-thousands column");
}

static void test_draw_text_phase_level_con(void)
{
    reset_sprites();
    cupcake_scoreboard_draw_text("P-3", capture_sprite, NULL);
    if (!has_sprite("digit40") || !has_sprite("digit41") || !has_sprite("digit42"))
        fail("text P-3: P segments");
    if (!has_sprite("digit32"))
        fail("text P-3: dash");
    if (!has_sprite("digit21") || !has_sprite("digit23"))
        fail("text P-3: digit 3");

    reset_sprites();
    cupcake_scoreboard_draw_text("L-2", capture_sprite, NULL);
    if (!has_sprite("digit44") || !has_sprite("digit45") || !has_sprite("digit46"))
        fail("text L-2: L segments");

    reset_sprites();
    cupcake_scoreboard_draw_text("CON", capture_sprite, NULL);
    if (!has_sprite("digit40") || !has_sprite("digit44") || !has_sprite("digit46"))
        fail("text CON: C segments");
    if (!has_sprite("digit30") || !has_sprite("digit31") || !has_sprite("digit33"))
        fail("text CON: O segments");
    if (!has_sprite("digit20") || !has_sprite("digit21") || !has_sprite("digit26"))
        fail("text CON: N segments");
}

static void test_draw_play_mode_score_and_phase(void)
{
    cupcake_play_state_t play;

    memset(&play, 0, sizeof play);
    play.mode = CUPCAKE_MODE_PLAY;
    play.scoreboard.score = 250;
    play.scoreboard.phase = 2;

    reset_sprites();
    cupcake_scoreboard_draw(&play, capture_sprite, NULL);
    if (!has_sprite("digit00") || !has_sprite("digit10"))
        fail("play draw: score digits");
    if (!has_sprite("digit40") || !has_sprite("digit42"))
        fail("play draw: phase overlay P-2");
}

static void test_draw_demo_con_overlay(void)
{
    cupcake_play_state_t play;

    memset(&play, 0, sizeof play);
    play.mode = CUPCAKE_MODE_DEMO;
    play.scoreboard.value = 100;
    play.scoreboard.show_con = 1;

    reset_sprites();
    cupcake_scoreboard_draw(&play, capture_sprite, NULL);
    if (!has_sprite("digit00") || !has_sprite("digit11"))
        fail("demo CON: hi-score digits remain");
    if (!has_sprite("digit40") || !has_sprite("digit30") || !has_sprite("digit20"))
        fail("demo CON: overlay letters");
}

static void test_start_intro_shows_hiscore_then_play_score(void)
{
    cupcake_play_state_t *p;
    cupcake_timers_t *tm;

    cupcake_init();
    p = cupcake_play_state();
    tm = cupcake_timers();
    p->level = 1;
    p->scoreboard.hi_score[1] = 4321;
    cupcake_on_start(1, 0);

    cupcake_timers_update(tm, 3.16f);
    if (p->scoreboard.score != 4321u)
        fail("start tick1: level hi-score on scoreboard.score");

    cupcake_timers_update(tm, 3.16f);
    if (p->mode != CUPCAKE_MODE_PLAY)
        fail("start tick2: enters play");
    if (p->scoreboard.score != 0u)
        fail("start tick2: run score reset");
    if (p->scoreboard.phase != 1)
        fail("start tick2: phase indicator set");
}

int main(void)
{
    test_draw_digits_full_score();
    test_draw_text_phase_level_con();
    test_draw_play_mode_score_and_phase();
    test_draw_demo_con_overlay();
    test_start_intro_shows_hiscore_then_play_score();

    if (g_fail)
        return 1;
    printf("cupcake_scoreboard: all tests passed\n");
    return 0;
}
