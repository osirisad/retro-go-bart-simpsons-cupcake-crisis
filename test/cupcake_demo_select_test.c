/*
 * TASK-09 demo Select level cycle — run: make test-demo-select
 */
#include "cupcake.h"
#include "cupcake_hiscore.h"
#include "cupcake_scoreboard.h"

#include <stdio.h>
#include <string.h>

static int g_fail;
static char g_sprites[32][16];
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
    if (!name || g_sprite_count >= 32)
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

static void test_select_cycle(void)
{
    const cupcake_state_t *st;

    cupcake_init();
    press_button(CUPCAKE_BTN_SELECT);
    st = cupcake_get_state();
    if (st->play.level != 1 || st->play.mode != CUPCAKE_MODE_DEMO)
        fail("select: level 0 -> 1 stays demo");
    if (st->play.scoreboard.level != 1)
        fail("select: scoreboard.level = 1");

    press_button(CUPCAKE_BTN_SELECT);
    st = cupcake_get_state();
    if (st->play.level != 2)
        fail("select: level 1 -> 2");

    press_button(CUPCAKE_BTN_SELECT);
    st = cupcake_get_state();
    if (st->play.level != 0 || st->play.mode != CUPCAKE_MODE_DEMO)
        fail("select: level 2 -> 0 restores attract");
    if (st->play.scoreboard.level != 0)
        fail("select: scoreboard.level cleared at attract");
    if (st->play.scoreboard.value != cupcake_hiscore_attract(&st->play))
        fail("select: hi-score value restored at level 0");
}

static void test_level_select_stops_timers_and_freezes_demo(void)
{
    cupcake_timers_t *tm;
    uint16_t frame_before;
    int i;

    cupcake_init();
    tm = cupcake_timers();
    for (i = 0; i < 10; i++)
        cupcake_update();
    frame_before = cupcake_get_state()->demo_frame;

    press_button(CUPCAKE_BTN_SELECT);
    if (cupcake_timer_active(tm, CUPCAKE_TMR_START))
        fail("level select: timers stopped");
    if (cupcake_timer_active(tm, CUPCAKE_TMR_GAME))
        fail("level select: game timer stopped");

    for (i = 0; i < 20; i++)
        cupcake_update();
    if (cupcake_get_state()->demo_frame != frame_before)
        fail("level select: demo replay frozen (SS.on=false)");
}

static void test_level_select_draws_label(void)
{
    cupcake_play_state_t play;

    memset(&play, 0, sizeof play);
    play.mode = CUPCAKE_MODE_DEMO;
    play.level = 1;
    play.scoreboard.level = 1;
    play.scoreboard.disp = CUPCAKE_SB_DISP_LEVEL;

    g_sprite_count = 0;
    cupcake_scoreboard_draw_demo_level(&play, capture_sprite, NULL);
    if (!has_sprite("digit44") || !has_sprite("digit45") || !has_sprite("digit46"))
        fail("level 1: L segments on column 4");
    if (!has_sprite("digit32"))
        fail("level 1: dash on column 3");
    if (!has_sprite("digit21") || !has_sprite("digit23"))
        fail("level 1: digit 1 on column 2");
}

static void test_attract_demo_resumes_after_cycle(void)
{
    uint16_t frame0;
    int i;

    cupcake_init();
    for (i = 0; i < 5; i++)
        cupcake_update();
    press_button(CUPCAKE_BTN_SELECT);
    press_button(CUPCAKE_BTN_SELECT);
    press_button(CUPCAKE_BTN_SELECT);

    frame0 = cupcake_get_state()->demo_frame;
    for (i = 0; i < 20; i++)
        cupcake_update();
    if (cupcake_get_state()->demo_frame == frame0)
        fail("attract restore: demo replay advances again");
}

int main(void)
{
    test_select_cycle();
    test_level_select_stops_timers_and_freezes_demo();
    test_level_select_draws_label();
    test_attract_demo_resumes_after_cycle();

    if (g_fail)
        return 1;
    printf("cupcake_demo_select: all tests passed\n");
    return 0;
}
