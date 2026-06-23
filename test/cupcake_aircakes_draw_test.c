/*
 * TASK-26 Draw visible flying aircakes — run: make test-aircakes-draw
 */
#include "cupcake_state.h"

#include <stdio.h>
#include <string.h>

static int g_fail;
static char g_sprites[16][16];
static int g_sprite_count;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void capture_sprite(const char *name, void *ctx)
{
    (void)ctx;
    if (!name || g_sprite_count >= 16)
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

static void test_sprite_names(void)
{
    char name[16];

    cupcake_aircake_sprite_name(0, name, sizeof name);
    if (strcmp(name, "cake0") != 0)
        fail("sprite name: cake0");
    cupcake_aircake_sprite_name(9, name, sizeof name);
    if (strcmp(name, "cake9") != 0)
        fail("sprite name: cake9");
    cupcake_aircake_sprite_name(8, name, sizeof name);
    if (strcmp(name, "cake8") != 0)
        fail("sprite name: cake8");
}

static void test_hidden_when_group_invisible(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_aircake_set_visible(&p.aircakes, 4, 1);
    p.aircakes.group_visible = 0;

    reset_sprites();
    cupcake_aircakes_draw_visible(&p, capture_sprite, NULL);
    if (g_sprite_count != 0)
        fail("draw: group hidden draws nothing");
}

static void test_draws_visible_cakes(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_aircake_set_visible(&p.aircakes, 2, 1);
    cupcake_aircake_set_visible(&p.aircakes, 7, 1);

    reset_sprites();
    cupcake_aircakes_draw_visible(&p, capture_sprite, NULL);

    if (!has_sprite("cake2"))
        fail("draw: cake2 visible");
    if (!has_sprite("cake7"))
        fail("draw: cake7 visible");
    if (has_sprite("cake3"))
        fail("draw: hidden cake3 not drawn");
}

static void test_maggie_throw_cake8(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_aircake_set_visible(&p.aircakes, 8, 1);

    reset_sprites();
    cupcake_aircakes_draw_visible(&p, capture_sprite, NULL);

    if (!has_sprite("cake8"))
        fail("draw: maggie throw cake8");
}

static void test_miss_cupcake_aircake0(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_bart_set_miss_pose(&p, 9);
    cupcake_aircake_set_visible(&p.aircakes, 0, 1);

    reset_sprites();
    cupcake_aircakes_draw_visible(&p, capture_sprite, NULL);

    if (!has_sprite("cake0"))
        fail("draw: miss cupcake aircake0");
}

static void test_miss_couch_hides_group(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_aircake_set_visible(&p.aircakes, 3, 1);
    p.aircakes.group_visible = 0;
    p.aircakes.visible = 0;

    reset_sprites();
    cupcake_aircakes_draw_visible(&p, capture_sprite, NULL);
    if (g_sprite_count != 0)
        fail("draw: couch miss hides aircakes group");
}

int main(void)
{
    test_sprite_names();
    test_hidden_when_group_invisible();
    test_draws_visible_cakes();
    test_maggie_throw_cake8();
    test_miss_cupcake_aircake0();
    test_miss_couch_hides_group();

    if (g_fail)
        return 1;
    printf("cupcake_aircakes_draw: all tests passed\n");
    return 0;
}
