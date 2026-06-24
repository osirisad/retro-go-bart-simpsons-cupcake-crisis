/*
 * TASK-44 demo replay regression — run: make test-demo-replay
 */
#include "cupcake.h"
#include "cupcake_demo.h"
#include "cupcake_sprites.h"

#include <stdio.h>
#include <string.h>

#define EXPECTED_DEMO_FRAMES 163

static int g_fail;
static unsigned g_draw_count;
static unsigned g_bad_draw;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static int draw_cb(cupcake_cb_type_t type, const char *str, int a0, int a1)
{
    (void)a0;
    (void)a1;
    if (type != CUPCAKE_CB_SPR || !str)
        return 0;
    g_draw_count++;
    if (!cupcake_sprite_by_name(str)) {
        g_bad_draw++;
        fprintf(stderr, "FAIL: demo draw missing sprite '%s'\n", str);
        g_fail = 1;
    }
    return 0;
}

static void test_frame_count_constant(void)
{
    if (CUPCAKE_DEMO_FRAME_COUNT != EXPECTED_DEMO_FRAMES)
        fail("demo: frame count must match demo.model (163)");
}

static void test_all_frame_sprites_blittable(void)
{
    unsigned f;
    unsigned i;

    for (f = 0; f < CUPCAKE_DEMO_FRAME_COUNT; f++) {
        const cupcake_demo_frame_t *fr = &cupcake_demo_frames[f];
        for (i = 0; i < fr->count; i++) {
            const char *name = cupcake_demo_sprite_name(fr->sprite_ids[i]);
            if (!name) {
                fail("demo: null sprite name in frame table");
                return;
            }
            if (!cupcake_sprite_by_name(name)) {
                fprintf(stderr, "FAIL: demo frame %u sprite '%s' not in atlas\n", f, name);
                g_fail = 1;
            }
        }
    }
}

static void test_full_cycle_updates(void)
{
    const cupcake_state_t *st;
    unsigned tick;
    const unsigned total_ticks = (unsigned)CUPCAKE_DEMO_FRAME_COUNT * 8u;

    cupcake_init();
    cupcake_set_callback(draw_cb);
    g_draw_count = 0;
    g_bad_draw = 0;

    for (tick = 0; tick < total_ticks; tick++) {
        cupcake_update();
        cupcake_draw();
    }

    st = cupcake_get_state();
    if (st->demo_frame != 0)
        fail("demo: full loop should wrap demo_frame to 0");
    if (g_draw_count == 0)
        fail("demo: draw pass produced no sprites");
    if (g_bad_draw != 0)
        fail("demo: draw referenced missing sprites");
}

int main(void)
{
    test_frame_count_constant();
    test_all_frame_sprites_blittable();
    test_full_cycle_updates();

    if (g_fail)
        return 1;
    printf("cupcake_demo_replay: all tests passed (%d frames)\n", EXPECTED_DEMO_FRAMES);
    return 0;
}
