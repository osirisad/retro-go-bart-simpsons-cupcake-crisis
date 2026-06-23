/*
 * Play-mode entity draw (maggie, couch, marge, pacifier, miss) — run: make test-entities-draw
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

static void test_maggie_draws_base_and_overlay(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.maggie.visible = 1;
    p.maggie.index = 2;

    reset_sprites();
    cupcake_maggie_draw_visible(&p, capture_sprite, NULL);
    if (!has_sprite("maggie0"))
        fail("maggie draw: always maggie0 base");
    if (!has_sprite("maggie2"))
        fail("maggie draw: throw overlay maggie2");
    if (g_sprite_count != 2)
        fail("maggie draw: base + one overlay");
}

static void test_maggie_hidden_when_invisible(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.maggie.index = 1;

    reset_sprites();
    cupcake_maggie_draw_visible(&p, capture_sprite, NULL);
    if (g_sprite_count != 0)
        fail("maggie draw: nothing when not visible");
}

static void test_couch_draws_visible_frames(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.couch.onscreen = 1;
    cupcake_couch_set_frame_visible(&p.couch, 0, 1);
    cupcake_couch_set_frame_visible(&p.couch, 2, 1);

    reset_sprites();
    cupcake_couch_draw_visible(&p, capture_sprite, NULL);
    if (!has_sprite("couch0") || !has_sprite("couch2"))
        fail("couch draw: visible frames");
}

static void test_marge_and_pacifier(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.marge.visible = 1;
    p.pacifier.visible = 1;
    p.pacifier.index = 2;

    reset_sprites();
    cupcake_marge_draw_visible(&p, capture_sprite, NULL);
    cupcake_pacifier_draw_visible(&p, capture_sprite, NULL);
    if (!has_sprite("marge1"))
        fail("marge draw: marge1");
    if (!has_sprite("pacifier2"))
        fail("pacifier draw: pacifier2");
}

static void test_miss_icons(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.miss.count = 2;

    reset_sprites();
    cupcake_miss_draw_visible(&p, capture_sprite, NULL);
    if (!has_sprite("miss1") || !has_sprite("miss2") || has_sprite("miss3"))
        fail("miss draw: miss1 and miss2 only");
}

int main(void)
{
    test_maggie_draws_base_and_overlay();
    test_maggie_hidden_when_invisible();
    test_couch_draws_visible_frames();
    test_marge_and_pacifier();
    test_miss_icons();

    if (g_fail)
        return 1;
    printf("cupcake_entities_draw: all tests passed\n");
    return 0;
}
