/*
 * Play-mode composite draw order (TASK-37) — run: make test-play-draw
 */
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

static int sprite_index(const char *name)
{
    int i;

    for (i = 0; i < g_sprite_count; i++)
        if (strcmp(g_sprites[i], name) == 0)
            return i;
    return -1;
}

static void expect_before(const char *earlier, const char *later)
{
    int a = sprite_index(earlier);
    int b = sprite_index(later);

    if (a < 0 || b < 0)
        return;
    if (a >= b)
        fail("draw order: expected layer before later layer");
}

static void reset_sprites(void)
{
    g_sprite_count = 0;
}

static void test_full_play_draw_order(void)
{
    cupcake_play_state_t p;
    char name[16];

    memset(&p, 0, sizeof p);
    p.grid.group_visible = 1;
    p.aircakes.group_visible = 1;
    p.couch.onscreen = 1;
    cupcake_couch_set_frame_visible(&p.couch, 1, 1);
    p.maggie.visible = 1;
    p.maggie.index = 1;
    p.marge.visible = 1;
    p.pacifier.visible = 1;
    p.pacifier.index = 1;
    p.miss.count = 1;
    p.bart.pos = 2;
    p.bart.visible = cupcake_bart_position_layers(2);
    cupcake_grid_set_visible(&p.grid, 1, 1, 1);
    cupcake_aircake_set_visible(&p.aircakes, 3, 1);

    cupcake_grid_sprite_name(1, 1, name, sizeof name);
    if (strcmp(name, "cake01") != 0)
        fail("setup: grid sprite name");

    reset_sprites();
    cupcake_play_draw_visible(&p, capture_sprite, NULL);

    if (sprite_index("couch1") < 0)
        fail("play draw: couch visible");
    if (sprite_index("maggie0") < 0 || sprite_index("maggie1") < 0)
        fail("play draw: maggie visible");
    if (sprite_index("marge1") < 0)
        fail("play draw: marge visible");
    if (sprite_index("pacifier1") < 0)
        fail("play draw: pacifier visible");
    if (sprite_index("cake01") < 0)
        fail("play draw: grid cupcake visible");
    if (sprite_index("cake3") < 0)
        fail("play draw: aircake visible");
    if (sprite_index("bart2") < 0)
        fail("play draw: bart visible");
    if (sprite_index("miss1") < 0)
        fail("play draw: miss icon visible");

    expect_before("couch1", "maggie0");
    expect_before("maggie1", "marge1");
    expect_before("marge1", "pacifier1");
    expect_before("pacifier1", "cake01");
    expect_before("cake01", "cake3");
    expect_before("cake3", "bart2");
    expect_before("bart2", "miss1");
}

static void test_couch_miss_draw_order(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.grid.group_visible = 0;
    p.aircakes.group_visible = 0;
    p.miss.count = 2;
    cupcake_bart_set_miss_pose(&p, 6);

    reset_sprites();
    cupcake_play_draw_visible(&p, capture_sprite, NULL);

    if (sprite_index("cake11") >= 0)
        fail("couch miss draw: grid hidden");
    if (sprite_index("cake0") >= 0)
        fail("couch miss draw: aircakes hidden");
    if (sprite_index("bart6") < 0)
        fail("couch miss draw: bart6 visible");
    if (sprite_index("miss2") < 0)
        fail("couch miss draw: miss icons visible");
    expect_before("bart6", "miss1");
}

int main(void)
{
    test_full_play_draw_order();
    test_couch_miss_draw_order();

    if (g_fail)
        return 1;
    printf("cupcake_play_draw: all tests passed\n");
    return 0;
}
