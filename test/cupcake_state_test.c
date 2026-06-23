/*
 * Standalone checks for cupcake_state — run: make test-state
 */
#include "cupcake_state.h"
#include "cupcake_timer.h"

#include <stdio.h>
#include <string.h>

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void test_grid_bits(void)
{
    cupcake_grid_state_t grid;

    cupcake_grid_clear(&grid);
    cupcake_grid_set_visible(&grid, 1, 1, 1);
    cupcake_grid_set_visible(&grid, 4, 3, 1);
    cupcake_grid_set_visible(&grid, 5, 5, 1);

    if (!cupcake_grid_is_visible(&grid, 1, 1))
        fail("grid: lane1 slot1");
    if (!cupcake_grid_is_visible(&grid, 4, 3))
        fail("grid: lane4 slot3");
    if (!cupcake_grid_is_visible(&grid, 5, 5))
        fail("grid: lane5 slot5 (cake45 row)");
    if (cupcake_grid_is_visible(&grid, 2, 1))
        fail("grid: unset cell should be hidden");
}

static void test_aircakes_bits(void)
{
    cupcake_aircakes_state_t ac;

    cupcake_aircakes_clear(&ac);
    cupcake_aircake_set_visible(&ac, 0, 1);
    cupcake_aircake_set_visible(&ac, 8, 1);
    if (!cupcake_aircake_is_visible(&ac, 0) || !cupcake_aircake_is_visible(&ac, 8))
        fail("aircakes: cake0/cake8 bits");
    cupcake_aircake_set_visible(&ac, 8, 0);
    if (cupcake_aircake_is_visible(&ac, 8))
        fail("aircakes: clear bit");
}

static void test_save_roundtrip(void)
{
    cupcake_state_t src;
    cupcake_state_t dst;
    cupcake_timers_t tm_src;
    cupcake_timers_t tm_dst;
    cupcake_timer_config_t cfg = {
        .rate_sec = 0.5f,
        .max_ticks = 3,
        .start_tick = 0,
    };

    memset(&src, 0, sizeof src);
    cupcake_play_state_reset(&src.play);
    src.play.mode = CUPCAKE_MODE_PLAY;
    src.play.enabled = 1;
    src.play.bart.pos = 3;
    src.play.bart.count = 2;
    src.play.couch.next = 27;
    src.play.marge.loop = 1;
    src.play.miss.count = 2;
    src.play.scoreboard.show_con = 1;
    src.demo_frame = 42;
    src.rng_state = 0xDEADBEEFu;

    cupcake_timers_init(&tm_src);
    cupcake_timer_start(&tm_src, CUPCAKE_TMR_GAME, &cfg);
    cupcake_timers_export(&tm_src, &src);

    memset(&dst, 0, sizeof dst);
    dst = src;
    cupcake_timers_init(&tm_dst);
    cupcake_timers_import(&tm_dst, &dst);

    if (dst.play.bart.pos != 3 || dst.play.couch.next != 27)
        fail("save blob: play fields");
    if (!tm_dst.named[CUPCAKE_TMR_GAME].active)
        fail("save blob: game timer active");
    if (tm_dst.named[CUPCAKE_TMR_GAME].tick != tm_src.named[CUPCAKE_TMR_GAME].tick)
        fail("save blob: timer tick");
}

static void test_phase_reset(void)
{
    cupcake_play_state_t p;

    cupcake_play_state_reset(&p);
    cupcake_play_state_start_phase(&p, 2);
    if (p.mode != CUPCAKE_MODE_PLAY || !p.enabled || p.bart.pos != 2 || p.bart.count != 0)
        fail("start_phase: bart lane/count");
    if (p.grid.visible != 0 || p.aircakes.visible != 0)
        fail("start_phase: clears grid/aircakes");
}

int main(void)
{
    test_grid_bits();
    test_aircakes_bits();
    test_save_roundtrip();
    test_phase_reset();

    if (g_fail)
        return 1;
    printf("cupcake_state: all tests passed\n");
    return 0;
}
