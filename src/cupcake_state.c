/*
 * Play-state layout + save/load helpers (TASK-03).
 */
#include "cupcake_state.h"
#include "cupcake_scoreboard.h"
#include "cupcake_hiscore.h"
#include "cupcake_rng.h"

#include <stdio.h>
#include <string.h>

static int grid_bit_index(int lane, int slot)
{
    if (lane < 1 || lane > CUPCAKE_GRID_LANES || slot < 1 || slot > CUPCAKE_GRID_SLOTS)
        return -1;
    return (lane - 1) * CUPCAKE_GRID_SLOTS + (slot - 1);
}

void cupcake_grid_clear(cupcake_grid_state_t *g)
{
    if (!g)
        return;
    g->visible = 0;
    g->group_visible = 0;
}

void cupcake_grid_set_visible(cupcake_grid_state_t *g, int lane, int slot, int on)
{
    int bit;

    if (!g)
        return;
    bit = grid_bit_index(lane, slot);
    if (bit < 0)
        return;
    if (on) {
        g->visible |= (1u << (unsigned)bit);
        g->group_visible = 1;
    } else
        g->visible &= ~(1u << (unsigned)bit);
}

int cupcake_grid_is_visible(const cupcake_grid_state_t *g, int lane, int slot)
{
    int bit;

    if (!g)
        return 0;
    bit = grid_bit_index(lane, slot);
    if (bit < 0)
        return 0;
    return (g->visible & (1u << (unsigned)bit)) ? 1 : 0;
}

void cupcake_grid_sprite_name(int lane, int slot, char *buf, size_t bufsz)
{
    static const char *const row[4][5] = {
        {"cake01", "cake02", "cake03", "cake04", "cake05"},
        {"cake11", "cake12", "cake13", "cake14", "cake15"},
        {"cake21", "cake22", "cake23", "cake24", "cake25"},
        {"cake31", "cake32", "cake33", "cake34", "cake35"},
    };
    static const char *const row5[5] = {
        "cake41", "cake42", "cake43", "cake44", "cake45",
    };

    if (!buf || bufsz == 0)
        return;
    buf[0] = '\0';
    if (lane == 5 && slot >= 1 && slot <= 5) {
        snprintf(buf, bufsz, "%s", row5[slot - 1]);
        return;
    }
    if (lane < 1 || lane > 4 || slot < 1 || slot > 5)
        return;
    snprintf(buf, bufsz, "%s", row[lane - 1][slot - 1]);
}

void cupcake_grid_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx)
{
    int lane, slot;
    char name[16];

    if (!p || !fn || !p->grid.group_visible)
        return;

    for (lane = 1; lane <= CUPCAKE_GRID_LANES; lane++) {
        for (slot = 1; slot <= CUPCAKE_GRID_SLOTS; slot++) {
            if (!cupcake_grid_is_visible(&p->grid, lane, slot))
                continue;
            cupcake_grid_sprite_name(lane, slot, name, sizeof name);
            if (name[0])
                fn(name, ctx);
        }
    }
}

void cupcake_aircakes_clear(cupcake_aircakes_state_t *a)
{
    if (!a)
        return;
    a->visible = 0;
    a->group_visible = 0;
}

void cupcake_aircake_set_visible(cupcake_aircakes_state_t *a, int index, int on)
{
    if (!a || index < 0 || index >= CUPCAKE_AIRCAKE_COUNT)
        return;
    if (on) {
        a->visible |= (uint16_t)(1u << (unsigned)index);
        a->group_visible = 1;
    } else
        a->visible &= (uint16_t)~(1u << (unsigned)index);
}

int cupcake_aircake_is_visible(const cupcake_aircakes_state_t *a, int index)
{
    if (!a || index < 0 || index >= CUPCAKE_AIRCAKE_COUNT)
        return 0;
    return (a->visible & (uint16_t)(1u << (unsigned)index)) ? 1 : 0;
}

void cupcake_aircake_sprite_name(int index, char *buf, size_t bufsz)
{
    if (!buf || bufsz == 0)
        return;
    if (index < 0 || index >= CUPCAKE_AIRCAKE_COUNT) {
        buf[0] = '\0';
        return;
    }
    snprintf(buf, bufsz, "cake%d", index);
}

void cupcake_aircakes_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx)
{
    int i;
    char name[16];

    if (!p || !fn || !p->aircakes.group_visible)
        return;

    for (i = 0; i < CUPCAKE_AIRCAKE_COUNT; i++) {
        if (!cupcake_aircake_is_visible(&p->aircakes, i))
            continue;
        cupcake_aircake_sprite_name(i, name, sizeof name);
        if (name[0])
            fn(name, ctx);
    }
}

void cupcake_play_state_reset(cupcake_play_state_t *p)
{
    if (!p)
        return;
    memset(p, 0, sizeof *p);
    p->phase_threshold = 10000;
    p->phase = 1;
    p->mode = CUPCAKE_MODE_DEMO;
    p->grid.group_visible = 1;
    p->aircakes.group_visible = 1;
    p->bart.pos = 2;
    p->bart.visible = cupcake_bart_position_layers(2);
    p->bart.miss_index = -1;
    p->scoreboard.phase = 1;
}

void cupcake_play_state_start_demo(cupcake_play_state_t *p)
{
    if (!p)
        return;
    p->mode = CUPCAKE_MODE_DEMO;
    p->enabled = 0;
    p->level = 0;
    p->game_tick = 0;
    p->scoreboard.show_con = 0;
    p->scoreboard.level = 0;
    p->scoreboard.disp = CUPCAKE_SB_DISP_VALUE;
    p->scoreboard.value = cupcake_hiscore_attract(p);
}

void cupcake_play_state_start_phase(cupcake_play_state_t *p, int bart_lane)
{
    if (!p)
        return;
    p->mode = CUPCAKE_MODE_PLAY;
    p->enabled = 1;
    p->game_tick = 0;
    cupcake_bart_start(p, bart_lane);
    cupcake_cupcakes_start(p);
    cupcake_aircakes_start(p);
    cupcake_maggie_start(p);
    cupcake_marge_start(p);
    cupcake_couch_start(p);
    cupcake_pacifier_start(p);
}

int cupcake_phase_clamp(int phase)
{
    if (phase < 1)
        return 1;
    if (phase > CUPCAKE_PHASE_MAX)
        return CUPCAKE_PHASE_MAX;
    return phase;
}

int cupcake_bart_stack_lane(int pos)
{
    /* JS: cupcakes[Math.min(4, position)][slot] — 0-based grid row, not Bart lane index.
     * Row 0=cake01 (lane 1) … row 4=cake41 (lane 5). pos 0→row 0; pos 1→row 1; … pos 5→row 4. */
    if (pos < 1)
        return 1;
    if (pos >= 5)
        return 5;
    return pos + 1;
}

uint16_t cupcake_bart_position_layers(int pos)
{
    static const uint16_t masks[] = {
        (uint16_t)((1u << 0) | (1u << 1)), /* 0: hand-off — bart0 + bart1 only */
        (uint16_t)((1u << 1) | (1u << 7)), /* 1: lane 1 — bart1 + bart7 */
        (uint16_t)(1u << 2),
        (uint16_t)(1u << 3),
        (uint16_t)((1u << 4) | (1u << 8)),
        (uint16_t)((1u << 5) | (1u << 8)),
    };

    if (pos < 0 || pos > 5)
        return 0;
    return masks[pos];
}

void cupcake_bart_set_position(cupcake_play_state_t *p, int pos)
{
    int old_lane, new_lane, slot;

    if (!p || pos < 0 || pos > 5)
        return;

    /* JS Bart.position setter: hide prior lane stack, then prior bart layers. */
    old_lane = cupcake_bart_stack_lane((int)p->bart.pos);
    for (slot = 1; slot <= CUPCAKE_GRID_SLOTS; slot++)
        cupcake_grid_set_visible(&p->grid, old_lane, slot, 0);

    p->bart.pos = (int8_t)pos;
    p->bart.visible = cupcake_bart_position_layers(pos);

    /* Held cupcakes: slots 1..count on lane min(4, position). */
    new_lane = cupcake_bart_stack_lane(pos);
    for (slot = 1; slot <= (int)p->bart.count && slot <= CUPCAKE_GRID_SLOTS; slot++)
        cupcake_grid_set_visible(&p->grid, new_lane, slot, 1);
}

int cupcake_bart_material_visible(const cupcake_play_state_t *p, int material_index)
{
    if (!p || material_index < 0 || material_index >= 10)
        return 0;
    return (p->bart.visible & (uint16_t)(1u << (unsigned)material_index)) ? 1 : 0;
}

void cupcake_bart_set_miss_pose(cupcake_play_state_t *p, int index)
{
    if (!p || (index != 6 && index != 9))
        return;
    p->bart.miss_index = (int8_t)index;
    p->bart.visible = (uint16_t)(1u << (unsigned)index);
}

void cupcake_bart_clear_miss_pose(cupcake_play_state_t *p)
{
    int pos;

    if (!p || p->bart.miss_index < 0)
        return;
    pos = (int)p->bart.pos;
    p->bart.miss_index = -1;
    cupcake_bart_set_position(p, pos);
}

int cupcake_bart_miss_index(const cupcake_play_state_t *p)
{
    if (!p)
        return -1;
    return (int)p->bart.miss_index;
}

void cupcake_bart_start(cupcake_play_state_t *p, int pos)
{
    if (!p)
        return;
    p->bart.count = 0;
    p->bart.miss_index = -1;
    if (pos >= 0)
        cupcake_bart_set_position(p, pos);
}

void cupcake_maggie_start(cupcake_play_state_t *p)
{
    if (!p)
        return;
    p->maggie.visible = 0;
    p->maggie.loop = 0;
    p->maggie.index = 0;
}

void cupcake_miss_start(cupcake_play_state_t *p, int initial)
{
    if (!p)
        return;
    if (initial < 0)
        initial = 0;
    if (initial > CUPCAKE_MISS_MAX)
        initial = CUPCAKE_MISS_MAX;
    p->miss.count = (uint8_t)initial;
}

void cupcake_miss_decrease(cupcake_play_state_t *p)
{
    if (!p)
        return;
    if (p->miss.count > 0)
        p->miss.count--;
}

void cupcake_cupcakes_start(cupcake_play_state_t *p)
{
    if (!p)
        return;
    cupcake_grid_clear(&p->grid);
}

void cupcake_aircakes_start(cupcake_play_state_t *p)
{
    if (!p)
        return;
    cupcake_aircakes_clear(&p->aircakes);
}

void cupcake_pacifier_start(cupcake_play_state_t *p)
{
    if (!p)
        return;
    p->pacifier.visible = 0;
    p->pacifier.index = 0;
    p->pacifier.loop = 1;
    p->pacifier.counter = 0;
    p->pacifier.next = p->record ? 15u : (uint16_t)cupcake_rand_span(20, 20);
}

void cupcake_marge_start(cupcake_play_state_t *p)
{
    if (!p)
        return;
    p->marge.visible = 0;
    p->marge.loop = 0;
}

void cupcake_couch_start(cupcake_play_state_t *p)
{
    if (!p)
        return;
    p->couch.onscreen = 0;
    p->couch.anim = 0;
    p->couch.frame_visible = 0;
    p->couch.counter = 0;
    p->couch.next = p->record ? 25u : (uint16_t)cupcake_rand_span(20, 20);
}

int cupcake_couch_sit_bonus_ticks(const cupcake_couch_state_t *c)
{
    if (!c)
        return 0;
    if (c->frame_visible & (1u << 3))
        return 1;
    if (c->frame_visible & (1u << 2))
        return 2;
    if (c->frame_visible & (1u << 1))
        return 4;
    return 0;
}

void cupcake_couch_set_frame_visible(cupcake_couch_state_t *c, int index, int on)
{
    if (!c || index < 0 || index > 4)
        return;
    if (on)
        c->frame_visible |= (uint8_t)(1u << (unsigned)index);
    else
        c->frame_visible &= (uint8_t)~(1u << (unsigned)index);
}

void cupcake_maggie_sprite_name(int index, char *buf, size_t bufsz)
{
    if (!buf || bufsz == 0)
        return;
    buf[0] = '\0';
    if (index < 0 || index > 3)
        return;
    snprintf(buf, bufsz, "maggie%d", index);
}

void cupcake_maggie_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx)
{
    char name[16];

    if (!p || !fn || !p->maggie.visible)
        return;

    /* JS Maggie.step always shows maggie0; maggie1..3 overlay when index > 0. */
    fn("maggie0", ctx);
    if (p->maggie.index > 0) {
        cupcake_maggie_sprite_name((int)p->maggie.index, name, sizeof name);
        if (name[0])
            fn(name, ctx);
    }
}

void cupcake_couch_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx)
{
    int i;
    char name[16];

    if (!p || !fn || !p->couch.onscreen)
        return;

    for (i = 0; i <= 4; i++) {
        if (!(p->couch.frame_visible & (1u << (unsigned)i)))
            continue;
        snprintf(name, sizeof name, "couch%d", i);
        fn(name, ctx);
    }
}

void cupcake_marge_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx)
{
    if (!p || !fn || !p->marge.visible)
        return;
    fn("marge1", ctx);
}

void cupcake_pacifier_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx)
{
    char name[16];

    if (!p || !fn || !p->pacifier.visible || p->pacifier.index < 1 || p->pacifier.index > 2)
        return;
    snprintf(name, sizeof name, "pacifier%d", (int)p->pacifier.index);
    fn(name, ctx);
}

void cupcake_miss_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx)
{
    int i;
    char name[16];

    if (!p || !fn)
        return;

    for (i = 1; i <= (int)p->miss.count && i <= 3; i++) {
        snprintf(name, sizeof name, "miss%d", i);
        fn(name, ctx);
    }
}

void cupcake_bart_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx)
{
    static const char *const names[] = {
        "bart0", "bart1", "bart2", "bart3", "bart4",
        "bart5", "bart6", "bart7", "bart8", "bart9",
    };
    int i;

    if (!p || !fn)
        return;

    for (i = 0; i < 10; i++) {
        if (p->bart.visible & (uint16_t)(1u << (unsigned)i))
            fn(names[i], ctx);
    }
}

void cupcake_play_draw_visible(const cupcake_play_state_t *p, cupcake_grid_draw_fn fn, void *ctx)
{
    if (!p || !fn)
        return;

    /* JS onSnapshot entity order: couch, maggie, marge, pacifier, cake, aircakes, bart, miss. */
    cupcake_couch_draw_visible(p, fn, ctx);
    cupcake_maggie_draw_visible(p, fn, ctx);
    cupcake_marge_draw_visible(p, fn, ctx);
    cupcake_pacifier_draw_visible(p, fn, ctx);

    if (p->bart.miss_index == 6 || p->bart.miss_index == 9) {
        if (p->aircakes.group_visible)
            cupcake_aircakes_draw_visible(p, fn, ctx);
        cupcake_bart_draw_visible(p, fn, ctx);
        cupcake_miss_draw_visible(p, fn, ctx);
        return;
    }

    if (p->grid.group_visible)
        cupcake_grid_draw_visible(p, fn, ctx);
    if (p->aircakes.group_visible)
        cupcake_aircakes_draw_visible(p, fn, ctx);
    cupcake_bart_draw_visible(p, fn, ctx);
    cupcake_miss_draw_visible(p, fn, ctx);
}

void cupcake_timer_export(const cupcake_timer_t *tm, cupcake_timer_saved_t *out)
{
    if (!tm || !out)
        return;
    memset(out, 0, sizeof *out);
    out->active = tm->active;
    out->paused = tm->paused;
    out->persist = (uint8_t)(tm->cfg.persist ? 1 : 0);
    out->elapsed = tm->elapsed;
    out->delay_remaining = tm->delay_remaining;
    out->rate_override = tm->rate_override;
    out->rate_sec = tm->cfg.rate_fn ? 0.f : tm->cfg.rate_sec;
    out->tick = tm->tick;
    out->start_tick = tm->cfg.start_tick;
    out->max_ticks = tm->cfg.max_ticks;
}

void cupcake_timer_import(cupcake_timer_t *tm, const cupcake_timer_saved_t *in)
{
    if (!tm || !in)
        return;
    memset(tm, 0, sizeof *tm);
    if (!in->active)
        return;
    tm->active = 1;
    tm->paused = in->paused;
    tm->elapsed = in->elapsed;
    tm->delay_remaining = in->delay_remaining;
    tm->rate_override = in->rate_override;
    tm->tick = in->tick;
    tm->cfg.rate_sec = in->rate_sec;
    tm->cfg.start_tick = in->start_tick;
    tm->cfg.max_ticks = in->max_ticks;
    tm->cfg.persist = in->persist;
    /* Callbacks (rate_fn, on_start, OT, on_end) must be re-bound by game code after load. */
}

void cupcake_timers_export(const cupcake_timers_t *src, cupcake_state_t *dst)
{
    int i;

    if (!src || !dst)
        return;
    for (i = 0; i < CUPCAKE_TMR_NAMED_COUNT; i++)
        cupcake_timer_export(&src->named[i], &dst->timers_named[i]);
    for (i = 0; i < CUPCAKE_TMR_SCHEDULE_MAX; i++)
        cupcake_timer_export(&src->schedule[i], &dst->timers_sched[i]);
    dst->timers_group_paused = src->group_paused;
}

void cupcake_timers_import(cupcake_timers_t *dst, const cupcake_state_t *src)
{
    int i;

    if (!dst || !src)
        return;
    cupcake_timers_init(dst);
    for (i = 0; i < CUPCAKE_TMR_NAMED_COUNT; i++)
        cupcake_timer_import(&dst->named[i], &src->timers_named[i]);
    for (i = 0; i < CUPCAKE_TMR_SCHEDULE_MAX; i++)
        cupcake_timer_import(&dst->schedule[i], &src->timers_sched[i]);
    dst->group_paused = src->timers_group_paused;
}
