/*
 * TASK-24 Draw all visible grid cupcakes — run: make test-cupcakes-draw
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

static void test_sprite_name_lane5(void)
{
    char name[16];

    cupcake_grid_sprite_name(5, 1, name, sizeof name);
    if (strcmp(name, "cake41") != 0)
        fail("sprite name: lane5 slot1 cake41");
    cupcake_grid_sprite_name(5, 5, name, sizeof name);
    if (strcmp(name, "cake45") != 0)
        fail("sprite name: lane5 slot5 cake45");
    cupcake_grid_sprite_name(4, 2, name, sizeof name);
    if (strcmp(name, "cake32") != 0)
        fail("sprite name: lane4 slot2 cake32");
}

static void test_hidden_when_group_invisible(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_grid_set_visible(&p.grid, 2, 1, 1);
    p.grid.group_visible = 0;

    reset_sprites();
    cupcake_grid_draw_visible(&p, capture_sprite, NULL);
    if (g_sprite_count != 0)
        fail("draw: group hidden draws nothing");
}

static void test_draws_floor_and_held_stack(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.bart.count = 2;
    cupcake_bart_set_position(&p, 2);
    cupcake_grid_set_visible(&p.grid, 1, 1, 1);

    reset_sprites();
    cupcake_grid_draw_visible(&p, capture_sprite, NULL);

    if (!has_sprite("cake11") || !has_sprite("cake12"))
        fail("draw: bart held stack on lane 2");
    if (!has_sprite("cake01"))
        fail("draw: floor cupcake on lane 1");
    if (has_sprite("cake31"))
        fail("draw: hidden lane 4 should not appear");
}

static void test_draws_lane5_row(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_grid_set_visible(&p.grid, 5, 3, 1);

    reset_sprites();
    cupcake_grid_draw_visible(&p, capture_sprite, NULL);

    if (!has_sprite("cake43"))
        fail("draw: lane5 row cake43");
}

static void test_slot1_floor_only(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    cupcake_grid_set_visible(&p.grid, 1, 1, 1);
    cupcake_grid_set_visible(&p.grid, 1, 3, 0);

    reset_sprites();
    cupcake_grid_draw_visible(&p, capture_sprite, NULL);

    if (!has_sprite("cake01"))
        fail("draw: floor slot1 cake01");
    if (has_sprite("cake03"))
        fail("draw: hidden upper slot not drawn");
}

int main(void)
{
    test_sprite_name_lane5();
    test_hidden_when_group_invisible();
    test_draws_floor_and_held_stack();
    test_draws_lane5_row();
    test_slot1_floor_only();

    if (g_fail)
        return 1;
    printf("cupcake_cupcakes_draw: all tests passed\n");
    return 0;
}
