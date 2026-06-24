/*
 * TASK-55 level indicator on scoreboard — run: make test-level-indicator
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

static void press_button(uint16_t btn)
{
    cupcake_set_buttons(0);
    cupcake_update();
    cupcake_set_buttons(btn);
    cupcake_update();
    cupcake_set_buttons(0);
    cupcake_update();
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

static void test_attract_hides_level(void)
{
    cupcake_play_state_t play;

    cupcake_init();
    if (cupcake_play_state()->scoreboard.level != 0)
        fail("attract: scoreboard.level hidden");

    memset(&play, 0, sizeof play);
    play.mode = CUPCAKE_MODE_DEMO;
    play.level = 0;
    play.scoreboard.value = 100;
    play.scoreboard.disp = CUPCAKE_SB_DISP_VALUE;
    g_sprite_count = 0;
    cupcake_scoreboard_draw(&play, capture_sprite, NULL);
    if (has_sprite("digit44"))
        fail("attract: no L overlay");
}

static void test_demo_select_shows_level(void)
{
    cupcake_play_state_t play;

    cupcake_init();
    press_button(CUPCAKE_BTN_SELECT);
    if (cupcake_play_state()->scoreboard.level != 1)
        fail("demo select: scoreboard.level = 1");

    play = *cupcake_play_state();
    g_sprite_count = 0;
    cupcake_scoreboard_draw(&play, capture_sprite, NULL);
    if (!has_sprite("digit44") || !has_sprite("digit21"))
        fail("demo select: L-1 overlay drawn");
}

static void test_quick_start_shows_level_during_intro(void)
{
    cupcake_play_state_t play;

    cupcake_init();
    press_button(CUPCAKE_BTN_LEVEL2);
    if (cupcake_play_state()->scoreboard.disp != CUPCAKE_SB_DISP_LEVEL)
        fail("quick start intro: level overlay");

    play = *cupcake_play_state();
    play.mode = CUPCAKE_MODE_START;
    g_sprite_count = 0;
    cupcake_scoreboard_draw(&play, capture_sprite, NULL);
    if (!has_sprite("digit44") || !has_sprite("digit22"))
        fail("quick start intro: L-2 overlay drawn");
}

static void test_quick_start_play_shows_score_not_level(void)
{
    cupcake_play_state_t play;

    cupcake_init();
    press_button(CUPCAKE_BTN_LEVEL2);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    if (cupcake_play_state()->scoreboard.disp != CUPCAKE_SB_DISP_SCORE)
        fail("quick start play: run score display");

    play = *cupcake_play_state();
    play.mode = CUPCAKE_MODE_PLAY;
    g_sprite_count = 0;
    cupcake_scoreboard_draw(&play, capture_sprite, NULL);
    if (!has_sprite("digit00"))
        fail("quick start play: score digits");
    if (has_sprite("digit44"))
        fail("quick start play: no level overlay");
}

static void test_normal_start_hides_level(void)
{
    cupcake_play_state_t *p;

    cupcake_init();
    p = cupcake_play_state();
    p->level = 1;
    press_button(CUPCAKE_BTN_SELECT);
    cupcake_on_start(1, 0);
    if (p->scoreboard.level != 0)
        fail("normal start: level overlay cleared");
    if (p->scoreboard.phase != 1)
        fail("normal start: phase overlay shown instead");
    if (p->scoreboard.disp != CUPCAKE_SB_DISP_PHASE)
        fail("normal start: phase display mode");
}

static void test_set_level_clamps(void)
{
    cupcake_play_state_t play;

    memset(&play, 0, sizeof play);
    cupcake_scoreboard_set_level(&play, 5);
    if (play.scoreboard.level != CUPCAKE_LEVEL_MAX)
        fail("set_level: clamp high");
    cupcake_scoreboard_set_level(&play, -1);
    if (play.scoreboard.level != 0)
        fail("set_level: clamp low to hidden");
}

static void test_level_overrides_phase_in_draw(void)
{
    cupcake_play_state_t play;

    memset(&play, 0, sizeof play);
    play.mode = CUPCAKE_MODE_PLAY;
    play.scoreboard.level = 1;
    play.scoreboard.phase = 3;
    play.scoreboard.disp = CUPCAKE_SB_DISP_LEVEL;

    g_sprite_count = 0;
    cupcake_scoreboard_draw(&play, capture_sprite, NULL);
    if (!has_sprite("digit44"))
        fail("draw: level shown");
    if (has_sprite("digit40") && has_sprite("digit42"))
        fail("draw: phase hidden when level set");
}

int main(void)
{
    test_attract_hides_level();
    test_demo_select_shows_level();
    test_quick_start_shows_level_during_intro();
    test_quick_start_play_shows_score_not_level();
    test_normal_start_hides_level();
    test_set_level_clamps();
    test_level_overrides_phase_in_draw();

    if (g_fail)
        return 1;
    printf("cupcake_level_indicator: all tests passed\n");
    return 0;
}
