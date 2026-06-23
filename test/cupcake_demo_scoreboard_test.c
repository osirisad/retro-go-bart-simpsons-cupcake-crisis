/*
 * TASK-08 demo hi-score scoreboard — run: make test-demo-scoreboard
 */
#include "cupcake.h"
#include "cupcake_scoreboard.h"
#include "cupcake_state.h"

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

static void test_slot_digits_match_js(void)
{
    if (cupcake_scoreboard_slot_digit(0, 2, 5, 0) != 0)
        fail("alwayson 0: ones");
    if (cupcake_scoreboard_slot_digit(0, 2, 5, 1) != 0)
        fail("alwayson 0: tens");
    if (cupcake_scoreboard_slot_digit(0, 2, 5, 2) != -1)
        fail("alwayson 0: third slot blank");

    if (cupcake_scoreboard_slot_digit(42, 2, 5, 0) != 2)
        fail("42: ones");
    if (cupcake_scoreboard_slot_digit(42, 2, 5, 1) != 4)
        fail("42: tens");
    if (cupcake_scoreboard_slot_digit(42, 2, 5, 2) != -1)
        fail("42: hundreds blank");

    if (cupcake_scoreboard_slot_digit(12345, 2, 5, 0) != 5)
        fail("12345: ones");
    if (cupcake_scoreboard_slot_digit(12345, 2, 5, 4) != 1)
        fail("12345: ten-thousands");
}

static void test_digit_sprite_names(void)
{
    char name[16];

    cupcake_scoreboard_digit_sprite(3, 1, name, sizeof name);
    if (strcmp(name, "digit31") != 0)
        fail("digit sprite name");
}

static void test_demo_draws_segments_for_hiscore(void)
{
    cupcake_play_state_t play;

    memset(&play, 0, sizeof play);
    play.mode = CUPCAKE_MODE_DEMO;
    play.level = 0;
    play.scoreboard.value = 10;

    g_sprite_count = 0;
    cupcake_scoreboard_draw_demo_hiscore(&play, capture_sprite, NULL);
    if (g_sprite_count < 4)
        fail("hi-score draw: expected multiple segment sprites");
    if (!has_sprite("digit00"))
        fail("hi-score draw: digit 0 segment");
    if (!has_sprite("digit11") || !has_sprite("digit13"))
        fail("hi-score draw: digit 1 on tens column");
}

static void press_select(void)
{
    cupcake_set_buttons(0);
    cupcake_update();
    cupcake_set_buttons(CUPCAKE_BTN_SELECT);
    cupcake_update();
    cupcake_set_buttons(0);
    cupcake_update();
}

static void test_demo_state_on_init(void)
{
    const cupcake_state_t *st;
    cupcake_play_state_t *p;

    cupcake_init();
    p = cupcake_play_state();
    p->scoreboard.hi_score[0] = 555;
    cupcake_play_state_start_demo(p);

    st = cupcake_get_state();
    if (st->play.mode != CUPCAKE_MODE_DEMO)
        fail("init: demo mode");
    if (st->play.level != 0)
        fail("init: level 0 in attract");
    if (st->play.scoreboard.level != 0)
        fail("init: scoreboard level hidden");
    if (st->play.scoreboard.value != 555u)
        fail("init: scoreboard value = hi-score");
}

static void test_on_demo_restores_hiscore(void)
{
    cupcake_play_state_t *p;

    cupcake_init();
    p = cupcake_play_state();
    p->scoreboard.hi_score[0] = 9876;
    press_select();
    press_select();
    press_select();
    if (p->mode != CUPCAKE_MODE_DEMO || p->level != 0)
        fail("select cycle: back to attract");
    if (p->scoreboard.value != 9876u)
        fail("on_demo: value restored from hi-score");
}

int main(void)
{
    test_slot_digits_match_js();
    test_digit_sprite_names();
    test_demo_draws_segments_for_hiscore();
    test_demo_state_on_init();
    test_on_demo_restores_hiscore();

    if (g_fail)
        return 1;
    printf("cupcake_demo_scoreboard: all tests passed\n");
    return 0;
}
