/*
 * TASK-59 headless smoke — run: make test-smoke
 */
#include "cupcake.h"

#include <stdio.h>

static int g_fail;
static int g_frames;
static int g_sprites;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static int smoke_cb(cupcake_cb_type_t type, const char *str, int a0, int a1)
{
    (void)str;
    (void)a0;
    (void)a1;
    if (type == CUPCAKE_CB_FRAME)
        g_frames++;
    if (type == CUPCAKE_CB_SPR)
        g_sprites++;
    return 0;
}

int main(void)
{
    int i;

    cupcake_init();
    cupcake_set_callback(smoke_cb);

    for (i = 0; i < 10; i++) {
        cupcake_update();
        cupcake_draw();
    }

    if (g_frames != 10)
        fail("smoke: expected 10 frame callbacks");
    if (cupcake_get_state()->play.mode != CUPCAKE_MODE_DEMO)
        fail("smoke: should stay in demo after 10 ticks");

    if (g_fail)
        return 1;
    printf("cupcake_smoke: all tests passed\n");
    return 0;
}
