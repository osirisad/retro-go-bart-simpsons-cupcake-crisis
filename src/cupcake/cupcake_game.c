/*
 * Portable game core — shared by PC (SDL) and retro-go hosts.
 */
#include "cupcake.h"
#include "cupcake_demo.h"
#include "cupcake_port.h"
#include "cupcake_sprite_lcd.h"
#include "cupcake_state.h"
#include "cupcake_timer.h"
#include "cupcake_rng.h"
#include "cupcake_scoreboard.h"
#include "cupcake_hiscore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(CUPCAKE_GNW)
#define CUPCAKE_DEV_LOG(...) ((void)0)
#else
#define CUPCAKE_DEV_LOG(...) fprintf(stderr, __VA_ARGS__)
#endif

typedef struct {
    uint32_t magic;
    uint32_t version;
    cupcake_state_t state;
} cupcake_save_blob_t;

static cupcake_state_t g;
static cupcake_timers_t g_timers;
static uint16_t g_buttons;
static uint16_t g_buttons_prev;
static cupcake_cb_t g_cb;

typedef struct {
    int phase;
    int show_level;
} cupcake_start_ctx_t;

static cupcake_start_ctx_t g_start_ctx;

typedef struct {
    int pending;
    int level;
    int phase;
    uint32_t points;
} cupcake_debug_start_cfg_t;

static cupcake_debug_start_cfg_t g_debug_start;

static int g_miss_cupcake_lane;
static int g_pending_phase_restart_finish;

static void cupcake_on_phase_start(int game_timer_start_tick);
static void cupcake_timers_restore(void);
static void cupcake_on_stop(void);
static void cupcake_on_m_(void);
static void tmr_game_ot(void *ctx, int tick);
static float tmr_game_rate(void *ctx, int tick);
static void tmr_start_seq_on_start(void *ctx, int tick);
static void tmr_start_seq_on_tick(void *ctx, int tick);
static void tmr_miss_cupcake_start(void *ctx, int tick);
static void tmr_miss_cupcake_ot(void *ctx, int tick);
static void tmr_miss_cupcake_end(void *ctx, int tick);
static void tmr_miss_couch_ot(void *ctx, int tick);
static void tmr_miss_couch_end(void *ctx, int tick);
static void tmr_couch_spawn_on_start(void *ctx, int tick);
static void tmr_couch_spawn_ot(void *ctx, int tick);
static void tmr_couch_spawn_end(void *ctx, int tick);
static void couch_entity_start(cupcake_play_state_t *p, int spawn_anim);
static void tmr_phase_complete_start(void *ctx, int tick);
static void tmr_phase_complete_end(void *ctx, int tick);
static void tmr_sit_bonus_on_start(void *ctx, int tick);
static void tmr_sit_bonus_end(void *ctx, int tick);
static void tmr_bart_action_on_start(void *ctx, int tick);
static void tmr_bart_action_on_end(void *ctx, int tick);
static void sched_on_m(void *ctx, int tick);
static void sched_phase_restart_finish(void *ctx, int tick);
static void cupcake_phase_restart_play(void);
static void cupcake_run_deferred_work(void);
static void sched_maggie_index_reset(void *ctx, int tick);
static void sched_marge_start_after_delivery(void *ctx, int tick);
static void cupcake_game_timer_start(int start_tick);
static void cupcake_stop(void);
static void cupcake_on_demo(void);
static void cupcake_on_select(void);
static void cupcake_on_select_over(void);
static void cupcake_on_action(void);
static void cupcake_bart_action(void);
static void cupcake_handle_input(void);

/* Host update rate (~30 Hz); demo.model 0.25s uses 8 ticks/frame. */
#define CUPCAKE_TICK_DT (1.f / 30.f)

#define CUPCAKE_PIN_MAX 128

typedef struct {
    char name[32];
    int lcd_x;
    int lcd_y;
} cupcake_pin_entry_t;

#if defined(CUPCAKE_GNW)
static int debug_pin_freeze_demo;
static int debug_pin_solo;
#else /* host tuning — lcd_tune.txt live reload, debug pin */
static char debug_pin_name[32];
static cupcake_pin_entry_t debug_pin_tune[CUPCAKE_PIN_MAX];
static int debug_pin_tune_count;
static int debug_pin_freeze_demo;
static int debug_pin_solo;

static cupcake_pin_entry_t g_lcd_tune_override[CUPCAKE_PIN_MAX];
static int g_lcd_tune_override_count;
static int g_lcd_tune_logged;

static int load_lcd_tune_entries(cupcake_pin_entry_t *out, int max_entries);
static void tune_file_path(char *buf, size_t bufsz);
#endif

/* ~30 Hz tick; demo.model rate 0.25s -> 8 ticks per frame */
#define CUPCAKE_DEMO_TICKS_PER_FRAME 8

/* Direct host calls (no variadic — Win64 ABI breaks va_list -> fixed callback). */
static int host_frame(void)
{
    return g_cb ? g_cb(CUPCAKE_CB_FRAME, NULL, 0, 0) : 0;
}

static int host_sprite(const char *name, int lcd_x, int lcd_y)
{
    return g_cb ? g_cb(CUPCAKE_CB_SPR, name, lcd_x, lcd_y) : 0;
}

static int host_sfx(const char *id)
{
    return g_cb ? g_cb(CUPCAKE_CB_SFX, id, 0, 0) : 0;
}

static void host_sfx_hook(const char *id)
{
    host_sfx(id);
}

static void host_on_score_change(void)
{
    cupcake_play_state_t *p = &g.play;

    cupcake_hiscore_on_score_change(p);
    if (p->mode == CUPCAKE_MODE_PLAY &&
        !cupcake_timer_active(&g_timers, CUPCAKE_TMR_PHASE) &&
        p->points >= (uint32_t)p->phase * p->phase_threshold)
        cupcake_on_phase_complete();
}

#if defined(CUPCAKE_GNW)

static int debug_pin_is_active(void)
{
    return 0;
}

static void refresh_lcd_tune_override(void)
{
}

static void draw_sprite_auto(const char *name)
{
    host_sprite(name, CUPCAKE_LCD_AUTO, CUPCAKE_LCD_AUTO);
}

static void draw_debug_pin(void)
{
}

void cupcake_log_sprite_lcd(const char *name)
{
    (void)name;
}

#else

static int debug_pin_is_active(void)
{
    return debug_pin_tune_count > 0 || debug_pin_name[0] != '\0';
}

static void refresh_lcd_tune_override(void)
{
    char path[512];
    int n;

    n = load_lcd_tune_entries(g_lcd_tune_override, CUPCAKE_PIN_MAX);
    if (!g_lcd_tune_logged) {
        g_lcd_tune_logged = 1;
        tune_file_path(path, sizeof path);
        if (n > 0)
            CUPCAKE_DEV_LOG("lcd_tune: %d sprites from %s (live reload)\n", n, path);
        else
            CUPCAKE_DEV_LOG(
                    "lcd_tune: not loaded (%s missing?) — using cupcake_sprite_lcd.h\n", path);
    }
    g_lcd_tune_override_count = n;
}

static int lcd_tune_lookup(const char *name, int *out_x, int *out_y)
{
    int i;

    if (!name || !out_x || !out_y)
        return 0;
    for (i = 0; i < g_lcd_tune_override_count; i++) {
        if (strcmp(g_lcd_tune_override[i].name, name) == 0) {
            *out_x = g_lcd_tune_override[i].lcd_x;
            *out_y = g_lcd_tune_override[i].lcd_y;
            return 1;
        }
    }
    return 0;
}

static void draw_sprite_auto(const char *name)
{
    int tx;
    int ty;

    if (lcd_tune_lookup(name, &tx, &ty))
        host_sprite(name, tx, ty);
    else
        host_sprite(name, CUPCAKE_LCD_AUTO, CUPCAKE_LCD_AUTO);
}

static void tune_file_path(char *buf, size_t bufsz)
{
    const char *base = getenv("CUPCAKE_ASSETS");
    if (!base || !base[0])
        base = "assets";
    snprintf(buf, bufsz, "%s/lcd_tune.txt", base);
}

static int parse_lcd_tune_line(const char *line, char *entry, size_t entry_sz, int *x, int *y)
{
    (void)entry_sz;

    if (!line || !line[0] || line[0] == '#')
        return 0;
    if (sscanf(line, " %31s %d %d", entry, x, y) != 3)
        return 0;
    if (entry[0] == '#')
        return 0;
    return 1;
}

/* Live override: assets/lcd_tune.txt lines like "marge1 200 73" (no rebuild needed). */
static int load_lcd_tune(const char *name, int *out_x, int *out_y)
{
    char path[512];
    char line[128];
    FILE *fp;

    tune_file_path(path, sizeof path);
    fp = fopen(path, "r");
    if (!fp)
        return 0;
    while (fgets(line, sizeof line, fp)) {
        char entry[32];
        int x = 0;
        int y = 0;
        if (!parse_lcd_tune_line(line, entry, sizeof entry, &x, &y))
            continue;
        {
            const char *a = entry;
            const char *b = name;
            while (*a && *b && *a == *b) {
                a++;
                b++;
            }
            if (!*a && !*b) {
                *out_x = x;
                *out_y = y;
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}

/* Load all sprites from lcd_tune.txt (re-read each frame while pinning). */
static int load_lcd_tune_entries(cupcake_pin_entry_t *out, int max_entries)
{
    char path[512];
    char line[128];
    FILE *fp;
    int n = 0;

    tune_file_path(path, sizeof path);
    fp = fopen(path, "r");
    if (!fp)
        return 0;
    while (fgets(line, sizeof line, fp)) {
        char entry[32];
        int x = 0;
        int y = 0;
        if (!parse_lcd_tune_line(line, entry, sizeof entry, &x, &y))
            continue;
        if (n >= max_entries) {
            CUPCAKE_DEV_LOG("lcd_tune.txt: only first %d sprites pinned (raise CUPCAKE_PIN_MAX)\n",
                    max_entries);
            break;
        }
        snprintf(out[n].name, sizeof out[n].name, "%s", entry);
        out[n].lcd_x = x;
        out[n].lcd_y = y;
        n++;
    }
    fclose(fp);
    return n;
}

void cupcake_log_sprite_lcd(const char *name)
{
    char path[512];
    int i;

    tune_file_path(path, sizeof path);

    if (!name || !name[0]) {
        cupcake_pin_entry_t tmp[CUPCAKE_PIN_MAX];
        int n = load_lcd_tune_entries(tmp, CUPCAKE_PIN_MAX);

        CUPCAKE_DEV_LOG("  %s (%d sprites):\n", path, n);
        for (i = 0; i < n; i++)
            CUPCAKE_DEV_LOG("    %s %d %d\n", tmp[i].name, tmp[i].lcd_x, tmp[i].lcd_y);
        return;
    }

    {
        const cupcake_sprite_lcd_t *lcd = cupcake_sprite_lcd_by_name(name);
        int tx = 0;
        int ty = 0;

        if (lcd) {
            int draw_y = cupcake_sprite_lcd_resolve_y(name, lcd->lcd_y);

            CUPCAKE_DEV_LOG("  %s in binary (cupcake_sprite_lcd.h): %d, %d\n", name, lcd->lcd_x,
                    lcd->lcd_y);
            if (draw_y != lcd->lcd_y)
                CUPCAKE_DEV_LOG("  %s header draws at Y %d (marge hair needs Y >= %d)\n", name,
                        draw_y, draw_y);
        } else
            CUPCAKE_DEV_LOG("  %s: not in cupcake_sprite_lcd.h\n", name);

        if (load_lcd_tune(name, &tx, &ty))
            CUPCAKE_DEV_LOG("  %s override (assets/lcd_tune.txt): %d, %d (used as-is)\n", name,
                    tx, ty);
        else
            CUPCAKE_DEV_LOG("  (no %s line for %s — using header)\n", path, name);
    }
}

static void draw_debug_pin(void)
{
    int tx;
    int ty;
    int i;

    if (debug_pin_tune_count > 0) {
        debug_pin_tune_count = load_lcd_tune_entries(debug_pin_tune, CUPCAKE_PIN_MAX);
        for (i = 0; i < debug_pin_tune_count; i++) {
            if (!cupcake_sprite_by_name(debug_pin_tune[i].name))
                continue;
            host_sprite(debug_pin_tune[i].name, debug_pin_tune[i].lcd_x,
                        debug_pin_tune[i].lcd_y);
        }
        return;
    }

    if (!debug_pin_name[0])
        return;
    if (load_lcd_tune(debug_pin_name, &tx, &ty))
        host_sprite(debug_pin_name, tx, ty);
    else
        draw_sprite_auto(debug_pin_name);
}

#endif /* !CUPCAKE_GNW */

static void draw_scoreboard_sprite(const char *name, void *ctx)
{
    (void)ctx;
    draw_sprite_auto(name);
}

/* Grid names from $P.Cupcakes — lane 1 uses cake01-05, lane 2 cake11-15, etc. */
static void draw_sprite_cb(const char *name, void *ctx)
{
    (void)ctx;
    draw_sprite_auto(name);
}

static void draw_bart_play(const cupcake_play_state_t *p)
{
    if (!p)
        return;
    cupcake_play_draw_visible(p, draw_sprite_cb, NULL);
}

static int btn_pressed(int index)
{
    unsigned mask = 1u << index;
    return (g_buttons & mask) && !(g_buttons_prev & mask);
}

static int btn_released(int index)
{
    unsigned mask = 1u << index;
    return !(g_buttons & mask) && (g_buttons_prev & mask);
}

/* JS: move + game OT run only in play with enabled true (pause clears enabled). */
static int cupcake_phase_celebration_active(void)
{
    return cupcake_timer_active(&g_timers, CUPCAKE_TMR_PHASE) ? 1 : 0;
}

/* Block move/action during phase music, P-N interstitial, and start intro. */
static int cupcake_phase_transition_locked(void)
{
    const cupcake_play_state_t *p = &g.play;

    if (p->mode == CUPCAKE_MODE_START)
        return 1;
    if (cupcake_phase_celebration_active())
        return 1;
    if (p->mode == CUPCAKE_MODE_PLAY && p->scoreboard.disp == CUPCAKE_SB_DISP_PHASE)
        return 1;
    return 0;
}

static int cupcake_play_active(void)
{
    const cupcake_play_state_t *p = &g.play;

    return p->mode == CUPCAKE_MODE_PLAY && p->enabled && !cupcake_phase_transition_locked();
}

static void cupcake_on_stop(void)
{
    g.play.enabled = 0;
}

void cupcake_on_game_over(void)
{
    cupcake_play_state_t *p = &g.play;

    host_sfx("stop");
    cupcake_timers_stop(&g_timers);
    host_sfx("over");
    p->bart.count = 0;
    p->mode = CUPCAKE_MODE_OVER;
    cupcake_on_stop();
    cupcake_hiscore_on_score_change(p);
    CUPCAKE_DEV_LOG( "Game over — press Select for CON (phase %u)\n", p->phase);
}

void cupcake_miss_increase(void)
{
    cupcake_play_state_t *p = &g.play;

    if (p->miss.count < CUPCAKE_MISS_MAX)
        p->miss.count++;
    if (p->miss.count >= CUPCAKE_MISS_MAX)
        cupcake_on_game_over();
    else
        cupcake_on_phase_restart();
}

static void cupcake_miss_sequence_begin(void)
{
    cupcake_play_state_t *p = &g.play;

    cupcake_pause();
    cupcake_timer_stop(&g_timers, CUPCAKE_TMR_COUCH);
    p->marge.visible = 0;
    p->pacifier.visible = 0;
}

void cupcake_pause(void)
{
    g.play.enabled = 0;
    if (cupcake_timer_active(&g_timers, CUPCAKE_TMR_GAME))
        cupcake_timer_pause(&g_timers, CUPCAKE_TMR_GAME);
    if (cupcake_timer_active(&g_timers, CUPCAKE_TMR_COUCH))
        cupcake_timer_pause(&g_timers, CUPCAKE_TMR_COUCH);
}

void cupcake_resume(void)
{
    cupcake_play_state_t *p = &g.play;

    if (p->mode != CUPCAKE_MODE_PLAY)
        return;
    if (cupcake_phase_transition_locked())
        return;
    if (cupcake_timer_active(&g_timers, CUPCAKE_TMR_GAME))
        cupcake_timer_resume(&g_timers, CUPCAKE_TMR_GAME);
    if (cupcake_timer_active(&g_timers, CUPCAKE_TMR_COUCH))
        cupcake_timer_resume(&g_timers, CUPCAKE_TMR_COUCH);
    p->enabled = 1;
}

int cupcake_is_paused(void)
{
    return g.play.mode == CUPCAKE_MODE_PLAY && !g.play.enabled;
}

float cupcake_game_tick_rate_sec(const cupcake_play_state_t *p)
{
    static const float level1[] = {0.f,       26.f / 30.f, 23.f / 30.f, 21.f / 30.f,
                                   19.f / 30.f, 17.f / 30.f, 15.f / 30.f};
    static const float level2[] = {0.f,       19.f / 30.f, 17.f / 30.f, 15.f / 30.f,
                                   13.f / 30.f, 11.f / 30.f, 9.f / 30.f};
    const float *table;
    int phase;
    float rate;

    if (!p)
        return 26.f / 30.f;

    table = (p->level == 2) ? level2 : level1;
    phase = cupcake_phase_clamp((int)p->phase);
    rate = table[phase];

    if (p->phase_threshold != 1000u && p->points >= (uint32_t)p->phase * p->phase_threshold * 9u / 10u &&
        p->points < (uint32_t)p->phase * p->phase_threshold)
        rate -= rate * 0.15f;

    return rate;
}

static float tmr_game_rate(void *ctx, int tick)
{
    (void)ctx;
    (void)tick;
    return cupcake_game_tick_rate_sec(&g.play);
}

static void tmr_game_ot(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;

    if (!cupcake_play_active() || cupcake_phase_celebration_active())
        return;

    p->game_tick = (uint32_t)tick;
    cupcake_cupcakes_step(p);
    cupcake_aircakes_step(p);
    cupcake_maggie_step(p);
    cupcake_marge_step(p, tick);
    cupcake_couch_step(p, tick);
    cupcake_pacifier_step(p);
}

static void cupcake_game_timer_start(int start_tick)
{
    cupcake_timer_config_t cfg;

    memset(&cfg, 0, sizeof cfg);
    cfg.rate_fn = tmr_game_rate;
    cfg.max_ticks = 0;
    cfg.start_tick = start_tick;
    cfg.on_tick = tmr_game_ot;
    cupcake_timer_start(&g_timers, CUPCAKE_TMR_GAME, &cfg);
}

static void sched_on_m(void *ctx, int tick)
{
    (void)ctx;
    (void)tick;
    cupcake_on_m_();
}

static void sched_maggie_index_reset(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;
    (void)tick;
    p->maggie.index = 0;
}

static int cupcake_couch_slot3_visible(const cupcake_play_state_t *p)
{
    if (!p)
        return 0;
    return (p->couch.frame_visible & (1u << 3)) ? 1 : 0;
}

static void tmr_couch_spawn_on_start(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;
    (void)tick;
    p->couch.onscreen = 1;
    p->couch.anim = 0;
    cupcake_couch_set_frame_visible(&p->couch, 0, 1);
}

static void tmr_couch_spawn_ot(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;
    if (tick < 1 || tick > 4)
        return;
    host_sfx("couch");
    cupcake_couch_set_frame_visible(&p->couch, tick, 1);
    p->couch.anim = (uint8_t)tick;
    if (tick == 3)
        p->maggie.visible = 0;
}

static void tmr_couch_spawn_end(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;
    (void)tick;
    if (p->bart.pos != 5)
        cupcake_on_m_couch();
}

static void couch_spawn_timer_start(void)
{
    cupcake_timer_config_t cfg;

    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 3.f;
    cfg.start_tick = 1;
    cfg.max_ticks = 4;
    cfg.on_start = tmr_couch_spawn_on_start;
    cfg.on_tick = tmr_couch_spawn_ot;
    cfg.on_end = tmr_couch_spawn_end;
    cupcake_timer_start(&g_timers, CUPCAKE_TMR_COUCH, &cfg);
}

static void couch_entity_start(cupcake_play_state_t *p, int spawn_anim)
{
    int can_anim;

    if (!p)
        return;
    can_anim = spawn_anim && !p->couch.onscreen;
    cupcake_timer_stop(&g_timers, CUPCAKE_TMR_COUCH);
    cupcake_couch_start(p);
    if (can_anim)
        couch_spawn_timer_start();
}

void cupcake_couch_step(cupcake_play_state_t *p, int tick)
{
    (void)tick;
    if (!p || p->couch.onscreen)
        return;
    p->couch.counter++;
    if (p->couch.counter >= p->couch.next && !p->marge.visible)
        couch_entity_start(p, 1);
}

void cupcake_maggie_step(cupcake_play_state_t *p)
{
    float delay;

    if (!p)
        return;

    p->maggie.visible = 0;

    if (!cupcake_couch_slot3_visible(p)) {
        p->maggie.index = p->maggie.loop;
        p->maggie.visible = 1;

        if (p->maggie.loop == 3) {
            cupcake_aircake_set_visible(&p->aircakes, 8, 1);
            delay = 0.75f * cupcake_game_tick_rate_sec(p);
            cupcake_timers_schedule(&g_timers, delay, sched_maggie_index_reset, NULL);
        }

        if (p->maggie.loop > 0)
            host_sfx("throw");

        p->maggie.loop++;
        if (p->maggie.loop >= 4)
            p->maggie.loop = 0;
    } else {
        p->maggie.loop = 0;
    }
}

static void sched_marge_start_after_delivery(void *ctx, int tick)
{
    (void)ctx;
    (void)tick;
    cupcake_marge_start(&g.play);
}

typedef struct {
    int five_delivery;
} marge_bonus_ctx_t;

static marge_bonus_ctx_t g_marge_bonus_ctx;

static void cupcake_bart_give_cupcakes_to_marge(cupcake_play_state_t *p)
{
    int pos;

    if (!p)
        return;
    pos = (int)p->bart.pos;
    p->bart.count = 0;
    cupcake_bart_set_position(p, pos);
}

static void marge_bonus_on_tick(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;
    marge_bonus_ctx_t *mc = (marge_bonus_ctx_t *)ctx;

    if (!mc || !mc->five_delivery || tick != 5)
        return;
    cupcake_bart_give_cupcakes_to_marge(p);
    cupcake_marge_start(p);
}

static void marge_bonus_on_end(void *ctx)
{
    cupcake_play_state_t *p = &g.play;
    marge_bonus_ctx_t *mc = (marge_bonus_ctx_t *)ctx;
    float delay;

    if (!mc || mc->five_delivery)
        return;
    cupcake_bart_give_cupcakes_to_marge(p);
    delay = 0.5f * cupcake_game_tick_rate_sec(p);
    cupcake_timers_schedule(&g_timers, delay, sched_marge_start_after_delivery, NULL);
}

int cupcake_marge_collect(cupcake_play_state_t *p)
{
    int count, extra, point_ticks;
    float rate;
    cupcake_scoreboard_bonus_cfg_t cfg;

    if (!p || !p->marge.visible || p->bart.count == 0 || p->bart.pos != 0)
        return 0;

    count = (int)p->bart.count;
    extra = (count == 5) ? 5 : 0;
    g_marge_bonus_ctx.five_delivery = extra ? 1 : 0;

    if (!extra)
        host_sfx("deliver");

    rate = extra ? 0.025f : 0.06f;
    point_ticks = count + extra;

    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = rate;
    cfg.point_ticks = point_ticks;
    cfg.increment = 100;
    cfg.play_points_sfx = extra ? 0 : 1;
    cfg.play_five_on_first = extra ? 1 : 0;
    cfg.on_tick = marge_bonus_on_tick;
    cfg.on_end = marge_bonus_on_end;
    cfg.user_ctx = &g_marge_bonus_ctx;

    cupcake_scoreboard_add_bonus_ex(p, &g_timers, &cfg);
    return 1;
}

void cupcake_marge_step(cupcake_play_state_t *p, int tick)
{
    if (!p)
        return;

    if (p->marge.visible) {
        if (p->marge.loop == 2) {
            p->marge.loop = 0;
            p->marge.visible = 0;
        } else {
            p->marge.loop++;
        }
    } else if (p->bart.count > 0 && tick > 6 &&
               (tick % 6 == 5 ||
                (tick % 6 == 0 && cupcake_rand(3) == 0)) &&
               !p->couch.onscreen) {
        p->marge.visible = 1;
        host_sfx("marge");
    }
}

static void cupcake_on_m_(void)
{
    cupcake_miss_increase();
}

static void tmr_miss_cupcake_start(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;
    int aircake;

    (void)ctx;
    (void)tick;
    host_sfx("miss");
    aircake = (g_miss_cupcake_lane == 1 || g_miss_cupcake_lane == 2) ? 0 : 9;
    cupcake_aircake_set_visible(&p->aircakes, aircake, 1);
}

static void tmr_miss_cupcake_ot(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;
    if (tick <= 0)
        return;
    p->grid.group_visible = 0;
    p->grid.visible = 0;
    cupcake_bart_set_miss_pose(p, 9);
}

static void tmr_miss_cupcake_end(void *ctx, int tick)
{
    (void)ctx;
    (void)tick;
    cupcake_timers_schedule(&g_timers, 1.75f, sched_on_m, NULL);
}

void cupcake_on_m_cupcake(int lane)
{
    cupcake_timer_config_t cfg;

    g_miss_cupcake_lane = lane;
    cupcake_miss_sequence_begin();
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 0.5f;
    cfg.max_ticks = 1;
    cfg.on_start = tmr_miss_cupcake_start;
    cfg.on_tick = tmr_miss_cupcake_ot;
    cfg.on_end = tmr_miss_cupcake_end;
    cupcake_timer_start(&g_timers, CUPCAKE_TMR_MISS, &cfg);
}

void cupcake_cupcakes_step(cupcake_play_state_t *p)
{
    int lane;
    int stack_lane;

    if (!p)
        return;

    stack_lane = cupcake_bart_stack_lane((int)p->bart.pos);

    for (lane = 1; lane <= 4; lane++) {
        int grid_lane = cupcake_bart_stack_lane(lane);

        if (!cupcake_grid_is_visible(&p->grid, grid_lane, 1))
            continue;
        if (p->bart.pos == lane || p->bart.pos == 0 || p->bart.pos == 5)
            continue;
        /* Held stack uses grid row stack_lane; slot 1 looks like a floor cupcake. */
        if (p->bart.count > 0 && grid_lane == stack_lane)
            continue;
        cupcake_grid_set_visible(&p->grid, grid_lane, 1, 0);
        cupcake_on_m_cupcake(lane);
    }
}

static void tmr_miss_couch_ot(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;
    if (tick > 0 && tick < 6 && p->bart.pos < 5) {
        cupcake_bart_set_position(p, (int)p->bart.pos + 1);
        host_sfx("move");
    }
}

static void tmr_miss_couch_end(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;
    (void)tick;
    p->grid.group_visible = 0;
    p->grid.visible = 0;
    p->aircakes.group_visible = 0;
    p->aircakes.visible = 0;
    cupcake_bart_set_miss_pose(p, 6);
    host_sfx("whoa");
    cupcake_timers_schedule(&g_timers, 2.f, sched_on_m, NULL);
}

void cupcake_on_m_couch(void)
{
    cupcake_timer_config_t cfg;
    int ticks = 6 - (int)g.play.bart.pos;

    if (ticks < 1)
        ticks = 1;

    cupcake_miss_sequence_begin();
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 0.4f;
    cfg.max_ticks = ticks;
    cfg.on_tick = tmr_miss_couch_ot;
    cfg.on_end = tmr_miss_couch_end;
    cupcake_timer_start(&g_timers, CUPCAKE_TMR_MISS, &cfg);
}

static void tmr_phase_complete_start(void *ctx, int tick)
{
    (void)ctx;
    (void)tick;
    host_sfx("stop");
    host_sfx("phase");
}

static void tmr_phase_complete_end(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;
    (void)tick;

    if (p->phase < CUPCAKE_PHASE_MAX)
        p->phase++;
    p->phase = (uint8_t)cupcake_phase_clamp((int)p->phase);
    cupcake_miss_decrease(p);
    cupcake_scoreboard_set_level(p, 0);
    cupcake_scoreboard_set_phase(p, (int)p->phase);
    cupcake_phase_restart_play();
}

void cupcake_on_phase_complete(void)
{
    cupcake_timer_config_t cfg;
    cupcake_play_state_t *p = &g.play;

    if (p->mode != CUPCAKE_MODE_PLAY)
        return;
    if (cupcake_timer_active(&g_timers, CUPCAKE_TMR_PHASE))
        return;

    /* Stop bonus/miss/action timers — their on_end paths call resume(). */
    cupcake_timer_stop(&g_timers, CUPCAKE_TMR_BONUS);
    p->scoreboard.bonus_active = 0;
    cupcake_timer_stop(&g_timers, CUPCAKE_TMR_MISS);
    cupcake_timer_stop(&g_timers, CUPCAKE_TMR_ACTION);

    cupcake_pause();
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 4.f;
    cfg.max_ticks = 1;
    cfg.on_start = tmr_phase_complete_start;
    cfg.on_end = tmr_phase_complete_end;
    cupcake_timer_start(&g_timers, CUPCAKE_TMR_PHASE, &cfg);
}

static void sched_phase_restart_finish(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;

    (void)ctx;
    (void)tick;

    p->scoreboard.score = p->points;
    p->grid.group_visible = 1;
    p->aircakes.group_visible = 1;
    cupcake_pacifier_start(p);
    cupcake_scoreboard_show_run_score(p);
    cupcake_resume();
}

static void cupcake_schedule_phase_restart_finish(void)
{
    if (cupcake_timers_schedule(&g_timers, 0.75f, sched_phase_restart_finish, NULL) < 0)
        sched_phase_restart_finish(NULL, 0);
}

/* JS onPhaseRestart body: onPhaseStart → pause → 0.75s → resume. */
static void cupcake_phase_restart_play(void)
{
    cupcake_timers_stop(&g_timers);
    cupcake_on_phase_start(0);
    cupcake_pause();
    host_sfx("stop");
    /*
     * Scheduling from sched_on_m reuses the active schedule slot and drops on_start.
     * Defer to post-update when called reentrantly from a timer callback.
     */
    if (cupcake_timers_in_callback())
        g_pending_phase_restart_finish = 1;
    else
        cupcake_schedule_phase_restart_finish();
}

static void cupcake_run_deferred_work(void)
{
    if (!g_pending_phase_restart_finish)
        return;
    g_pending_phase_restart_finish = 0;
    cupcake_schedule_phase_restart_finish();
}

void cupcake_on_phase_restart(void)
{
    cupcake_play_state_t *p = &g.play;

    /* JS onPhaseRestart ordering (miss recovery — entities reset before interstitial). */
    cupcake_scoreboard_set_level(p, 0);
    cupcake_scoreboard_set_phase(p, (int)p->phase);
    cupcake_phase_restart_play();
}

static void tmr_sit_bonus_on_start(void *ctx, int tick)
{
    (void)ctx;
    (void)tick;
    host_sfx("bonus");
}

static void tmr_sit_bonus_end(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;
    int bonus_ticks;

    (void)ctx;
    (void)tick;

    bonus_ticks = cupcake_couch_sit_bonus_ticks(&p->couch);
    couch_entity_start(p, 0);
    cupcake_resume();
    if (bonus_ticks > 0)
        cupcake_scoreboard_add_bonus(0.03f, bonus_ticks, 100);
}

static void cupcake_bart_sit(cupcake_play_state_t *p)
{
    int bonus_ticks;

    if (!p || p->bart.pos != 4 || !p->couch.onscreen)
        return;

    /* Stop couch spawn animation so onEnd cannot fire onM_Couch after sit. */
    cupcake_timer_stop(&g_timers, CUPCAKE_TMR_COUCH);

    cupcake_bart_set_position(p, 5);
    cupcake_pause();

    if (p->couch.frame_visible & (1u << 4)) {
        couch_entity_start(p, 0);
        cupcake_resume();
        return;
    }

    bonus_ticks = cupcake_couch_sit_bonus_ticks(&p->couch);
    if (bonus_ticks <= 0) {
        couch_entity_start(p, 0);
        cupcake_resume();
        return;
    }

    cupcake_bart_sit_bonus(bonus_ticks);
}

static void tmr_bart_action_on_start(void *ctx, int tick)
{
    (void)ctx;
    (void)tick;
    cupcake_bart_set_position(&g.play, 0);
}

static void tmr_bart_action_on_end(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;
    int collected;

    (void)ctx;
    (void)tick;

    collected = cupcake_marge_collect(p);
    if (p->bart.pos == 0) {
        if (collected)
            p->bart.count = cupcake_grid_is_visible(&p->grid, cupcake_bart_stack_lane(1), 1) ? 1u : 0u;
        cupcake_bart_set_position(p, 1);
    }
}

void cupcake_bart_sit_bonus(int couch_bonus_points)
{
    cupcake_timer_config_t cfg;

    if (couch_bonus_points <= 0)
        return;

    cupcake_pause();
    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 2.f;
    cfg.max_ticks = 1;
    cfg.on_start = tmr_sit_bonus_on_start;
    cfg.on_end = tmr_sit_bonus_end;
    cupcake_timer_start(&g_timers, CUPCAKE_TMR_ACTION, &cfg);
}

void cupcake_add_points(int points)
{
    cupcake_play_state_t *p = &g.play;

    if (points <= 0)
        return;

    p->points += (uint32_t)points;
    p->scoreboard.score = p->points;
    cupcake_scoreboard_show_run_score(p);
    host_on_score_change();
}

void cupcake_set_threshold(uint32_t threshold)
{
    cupcake_play_state_t *p = &g.play;

    if (threshold == 0)
        threshold = 10000;
    p->phase_threshold = threshold;
}

void cupcake_set_record(int on)
{
    g.play.record = on ? 1u : 0u;
}

int cupcake_record_mode(void)
{
    return g.play.record ? 1 : 0;
}

uint32_t cupcake_phase_score_target(const cupcake_play_state_t *play)
{
    if (!play)
        return 0;
    return (uint32_t)play->phase * play->phase_threshold;
}

void cupcake_scoreboard_add_bonus(float rate_sec, int point_ticks, int increment)
{
    cupcake_scoreboard_bonus_cfg_t cfg;

    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = rate_sec;
    cfg.point_ticks = point_ticks;
    cfg.increment = increment;
    cfg.play_points_sfx = 1;
    cupcake_scoreboard_add_bonus_ex(&g.play, &g_timers, &cfg);
}

static void cupcake_on_select_over(void)
{
    g.play.scoreboard.show_con = 1;
    g.play.mode = CUPCAKE_MODE_DEMO;
    cupcake_on_stop();
    CUPCAKE_DEV_LOG( "CON — press Action to continue at phase %u\n", g.play.phase);
}

static void cupcake_stop(void)
{
    host_sfx("stop");
    cupcake_timers_stop(&g_timers);
    cupcake_on_stop();
}

static void cupcake_on_demo(void)
{
    cupcake_play_state_t *p = &g.play;

    cupcake_stop();
    cupcake_play_state_start_demo(p);
    cupcake_scoreboard_set_level(p, 0);
    cupcake_scoreboard_show_value(p, cupcake_hiscore_attract(p));
    g.demo_frame = 0;
    g.demo_tick = 0;
}

static void cupcake_on_select(void)
{
    cupcake_play_state_t *p = &g.play;

    if (p->mode == CUPCAKE_MODE_OVER) {
        cupcake_on_select_over();
        return;
    }

    if (p->mode != CUPCAKE_MODE_DEMO)
        return;

    p->level = (uint8_t)((p->level + 1) % 3);
    if (p->level > 0) {
        cupcake_stop();
        p->mode = CUPCAKE_MODE_DEMO;
        cupcake_scoreboard_set_level(p, (int)p->level);
    } else {
        cupcake_scoreboard_show_value(p, cupcake_hiscore_attract(p));
        cupcake_on_demo();
    }
}

static int bart_move_target(int pos, cupcake_move_t dir)
{
    switch (dir) {
    case CUPCAKE_MOVE_LEFT:
        return pos > 1 ? pos - 1 : -1;
    case CUPCAKE_MOVE_RIGHT:
        return pos < 4 ? pos + 1 : -1;
    default:
        return -1;
    }
}

int cupcake_bart_catch_cupcake(cupcake_play_state_t *p, int lane)
{
    if (!p || p->bart.count >= 5)
        return -1;
    p->bart.count++;
    host_sfx("cupcake");
    cupcake_add_points(100);
    cupcake_bart_set_position(p, lane);
    return (int)p->bart.count;
}

void cupcake_bart_catch_pacifier(cupcake_play_state_t *p)
{
    if (!p)
        return;
    host_sfx("pacifier2");
    cupcake_add_points(300);
    cupcake_pacifier_start(p);
}

void cupcake_aircakes_land_at_lane(cupcake_play_state_t *p, int lane)
{
    if (!p || lane < 1 || lane > 4)
        return;
    if (p->bart.pos == lane) {
        if (p->bart.count < 5)
            cupcake_bart_catch_cupcake(p, lane);
        else
            cupcake_on_m_cupcake(lane);
    } else {
        cupcake_grid_set_visible(&p->grid, cupcake_bart_stack_lane(lane), 1, 1);
    }
}

static void aircake_step_land(cupcake_play_state_t *p, int index)
{
    if (!p || !cupcake_aircake_is_visible(&p->aircakes, index))
        return;
    cupcake_aircake_set_visible(&p->aircakes, index, 0);
    cupcake_aircakes_land_at_lane(p, index);
}

static void aircake_step_advance(cupcake_play_state_t *p, int index, int next_index)
{
    if (!p || !cupcake_aircake_is_visible(&p->aircakes, index))
        return;
    cupcake_aircake_set_visible(&p->aircakes, index, 0);
    cupcake_aircake_set_visible(&p->aircakes, next_index, 1);
    host_sfx("step");
}

static void aircake_step_pick(cupcake_play_state_t *p, int index, const int *choices, int count)
{
    int next;

    if (!p || !choices || count <= 0 || !cupcake_aircake_is_visible(&p->aircakes, index))
        return;
    cupcake_aircake_set_visible(&p->aircakes, index, 0);
    next = cupcake_rand_pick(choices, count);
    cupcake_aircake_set_visible(&p->aircakes, next, 1);
    host_sfx("step");
}

void cupcake_aircakes_step(cupcake_play_state_t *p)
{
    static const int cake6_next[] = {5, 2};
    static const int cake7_next[] = {6, 6, 3};
    static const int cake8_early[] = {7, 7, 4};
    static const int cake8_late[] = {7, 7, 4, 4};

    if (!p)
        return;

    aircake_step_land(p, 1);
    aircake_step_land(p, 2);
    aircake_step_land(p, 3);
    aircake_step_land(p, 4);
    aircake_step_advance(p, 5, 1);
    aircake_step_pick(p, 6, cake6_next, 2);
    aircake_step_pick(p, 7, cake7_next, 3);
    if (p->phase <= 3)
        aircake_step_pick(p, 8, cake8_early, 3);
    else
        aircake_step_pick(p, 8, cake8_late, 4);
}

void cupcake_pacifier_on_loop3(cupcake_play_state_t *p)
{
    if (!p)
        return;
    if (p->bart.pos == 2)
        cupcake_bart_catch_pacifier(p);
    else
        cupcake_pacifier_start(p);
}

void cupcake_pacifier_step(cupcake_play_state_t *p)
{
    if (!p)
        return;

    p->pacifier.counter++;
    if (p->pacifier.counter < p->pacifier.next)
        return;

    if (p->pacifier.loop < 3) {
        p->pacifier.index = (uint8_t)(p->pacifier.loop == 1 ? 1 : 2);
        p->pacifier.visible = 1;
    }

    if (p->pacifier.loop == 1)
        host_sfx("pacifier1");

    if (p->pacifier.loop == 3)
        cupcake_pacifier_on_loop3(p);
    else
        p->pacifier.loop++;
}

static void cupcake_bart_move(cupcake_play_state_t *p, cupcake_move_t dir)
{
    int dest;
    int floor_lane;

    if (!p)
        return;

    dest = bart_move_target((int)p->bart.pos, dir);
    if (dest < 0)
        return;

    if (p->pacifier.visible && p->pacifier.index == 2 && p->bart.pos == 2)
        cupcake_bart_catch_pacifier(p);

    if (cupcake_marge_collect(p)) {
        if (dir == CUPCAKE_MOVE_RIGHT && dest == 1 &&
            cupcake_grid_is_visible(&p->grid, cupcake_bart_stack_lane(1), 1))
            p->bart.count = 1;
        else
            p->bart.count = 0;
    }

    /* JS: cupcakes[dest][1] — grid row is stack_lane(dest), not dest itself. */
    floor_lane = cupcake_bart_stack_lane(dest);
    if (p->bart.pos != 5 && cupcake_grid_is_visible(&p->grid, floor_lane, 1)) {
        if (p->bart.count < 5) {
            cupcake_grid_set_visible(&p->grid, floor_lane, 1, 0);
            cupcake_bart_catch_cupcake(p, dest);
        } else {
            cupcake_bart_set_position(p, dest);
            cupcake_on_m_cupcake(dest);
        }
    } else {
        cupcake_bart_set_position(p, dest);
        host_sfx("move");
    }
}

void cupcake_on_move(cupcake_move_t dir)
{
    cupcake_play_state_t *p = &g.play;

    if (!cupcake_play_active())
        return;

    if (dir == CUPCAKE_MOVE_UP || dir == CUPCAKE_MOVE_DOWN) {
        if (p->bart.pos == 4 && p->couch.onscreen)
            cupcake_bart_sit(p);
        return;
    }

    cupcake_bart_move(p, dir);
}

void cupcake_set_debug_start(int level, int phase, uint32_t points)
{
    if (level < 1)
        level = 1;
    if (level > CUPCAKE_LEVEL_MAX)
        level = CUPCAKE_LEVEL_MAX;
    if (phase < 1)
        phase = 1;
    if (phase > CUPCAKE_PHASE_MAX)
        phase = CUPCAKE_PHASE_MAX;

    g_debug_start.pending = 1;
    g_debug_start.level = level;
    g_debug_start.phase = phase;
    g_debug_start.points = points;
}

int cupcake_debug_start_pending(void)
{
    return g_debug_start.pending;
}

void cupcake_apply_debug_start(void)
{
    cupcake_play_state_t *p = &g.play;
    int level;
    int phase;
    uint32_t points;

    if (!g_debug_start.pending)
        return;

    level = g_debug_start.level;
    phase = g_debug_start.phase;
    points = g_debug_start.points;
    g_debug_start.pending = 0;

    cupcake_stop();
    p->level = (uint8_t)level;
    p->phase = (uint8_t)phase;
    p->points = points;
    p->scoreboard.score = points;
    p->scoreboard.value = points;
    p->scoreboard.show_con = 0;
    cupcake_scoreboard_set_level(p, 0);
    p->scoreboard.phase = (uint8_t)cupcake_phase_clamp(phase);
    cupcake_set_threshold(10000);
    host_sfx("stop");
    cupcake_on_phase_start(1);
    cupcake_scoreboard_show_run_score(p);
    cupcake_resume();
    host_on_score_change();

    CUPCAKE_DEV_LOG( "Debug start: level %d phase %d score %u (target %u)\n", level, phase,
            (unsigned)points, (unsigned)cupcake_phase_score_target(p));
}

void cupcake_debug_cheat_phase(int phase_1_to_6)
{
    cupcake_play_state_t *p = &g.play;

    if (phase_1_to_6 < 1)
        phase_1_to_6 = 1;
    if (phase_1_to_6 > CUPCAKE_PHASE_MAX)
        phase_1_to_6 = CUPCAKE_PHASE_MAX;
    if (p->mode != CUPCAKE_MODE_PLAY && p->mode != CUPCAKE_MODE_START)
        return;

    cupcake_timers_stop(&g_timers);
    host_sfx("stop");
    p->phase = (uint8_t)(phase_1_to_6 - 1);
    p->points = (uint32_t)(phase_1_to_6 - 1) * p->phase_threshold;
    cupcake_scoreboard_set_level(p, 0);
    cupcake_scoreboard_set_phase(p, phase_1_to_6);
    cupcake_on_phase_complete();

    CUPCAKE_DEV_LOG( "Debug cheat: jump to phase %d\n", phase_1_to_6);
}

void cupcake_on_quick_start(int level)
{
    cupcake_play_state_t *p = &g.play;

    if (level < 1)
        level = 1;
    if (level > CUPCAKE_LEVEL_MAX)
        level = CUPCAKE_LEVEL_MAX;

    cupcake_stop();
    p->level = (uint8_t)level;
    cupcake_scoreboard_set_level(p, level);
    cupcake_on_start(1, 1);
    CUPCAKE_DEV_LOG( "Quick start level %d\n", level);
}

static void cupcake_bart_action(void)
{
    cupcake_play_state_t *p = &g.play;
    cupcake_timer_config_t cfg;
    float rate;

    if (!cupcake_play_active() || p->bart.pos != 1)
        return;

    rate = cupcake_game_tick_rate_sec(p);
    rate *= p->marge.visible ? 0.5f : 0.8f;

    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = rate;
    cfg.max_ticks = 1;
    cfg.on_start = tmr_bart_action_on_start;
    cfg.on_end = tmr_bart_action_on_end;
    cupcake_timer_start(&g_timers, CUPCAKE_TMR_ACTION, &cfg);
}

static void cupcake_on_action(void)
{
    cupcake_play_state_t *p = &g.play;

    if (p->scoreboard.show_con) {
        cupcake_on_start((int)p->phase, 0);
        return;
    }
    if (p->mode == CUPCAKE_MODE_DEMO && p->level > 0) {
        /* TASK-47: demo Action → onStart(1), never direct play. */
        cupcake_on_start(1, 0);
        return;
    }
    if (p->mode == CUPCAKE_MODE_DEMO && p->level == 0) {
        CUPCAKE_DEV_LOG( "Press Select (X) to cycle level, Action (Z) to start\n");
        return;
    }
    if (cupcake_play_active()) {
        cupcake_bart_action();
        return;
    }
}

static void cupcake_handle_input(void)
{
    if (btn_pressed(0))
        cupcake_on_move(CUPCAKE_MOVE_LEFT);
    if (btn_pressed(1))
        cupcake_on_move(CUPCAKE_MOVE_RIGHT);
    if (btn_pressed(2))
        cupcake_on_move(CUPCAKE_MOVE_UP);
    if (btn_pressed(3))
        cupcake_on_move(CUPCAKE_MOVE_DOWN);
    if (btn_pressed(4))
        cupcake_on_action();
    if (btn_pressed(5))
        cupcake_on_select();
    if (btn_pressed(6))
        cupcake_on_quick_start(1);
    if (btn_pressed(7))
        cupcake_on_quick_start(2);
    if (btn_released(8) && g_cb)
        g_cb(CUPCAKE_CB_SOUND_TOGGLE, NULL, 0, 0);
}

static void cupcake_on_phase_start(int game_timer_start_tick)
{
    cupcake_play_state_t *p = &g.play;

    /* JS onPhaseStart: bart, maggie(1), cupcakes, aircakes, pacifier, marge, couch. */
    p->mode = CUPCAKE_MODE_PLAY;
    p->enabled = 0;
    p->game_tick = 0;
    p->scoreboard.score = p->points;
    cupcake_bart_start(p, 2);
    cupcake_maggie_start(p);
    cupcake_cupcakes_start(p);
    cupcake_aircakes_start(p);
    cupcake_pacifier_start(p);
    cupcake_marge_start(p);
    couch_entity_start(p, 0);
    cupcake_game_timer_start(game_timer_start_tick);
    CUPCAKE_DEV_LOG( "Phase start: level %u phase %u (play mode)\n", p->level, p->phase);
}

static void tmr_start_seq_on_start(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;
    const int phase = g_start_ctx.phase > 0 ? g_start_ctx.phase : 1;

    (void)ctx;
    (void)tick;

    p->phase = (uint8_t)cupcake_phase_clamp(phase);
    if (g_start_ctx.show_level) {
        cupcake_scoreboard_set_level(p, (int)p->level);
    } else {
        cupcake_scoreboard_set_level(p, 0);
        cupcake_scoreboard_set_phase(p, phase);
    }

    p->points = 0;
    cupcake_bart_start(p, 2);
    cupcake_maggie_start(p);
    cupcake_miss_start(p, 0);
    host_sfx("start");
}

static void tmr_start_seq_on_tick(void *ctx, int tick)
{
    cupcake_play_state_t *p = &g.play;
    const int phase = g_start_ctx.phase > 0 ? g_start_ctx.phase : 1;
    uint8_t lvl;

    (void)ctx;

    if (tick == 1) {
        lvl = p->level;
        if (lvl > CUPCAKE_LEVEL_MAX)
            lvl = 0;
        p->scoreboard.score = p->scoreboard.hi_score[lvl];
        cupcake_scoreboard_show_run_score(p);
    } else if (tick == 2) {
        p->scoreboard.value = 0;
        cupcake_on_phase_start(phase > 0 ? 0 : 1);
        cupcake_scoreboard_show_run_score(p);
        cupcake_resume();
    }
}

static void cupcake_timers_restore(void)
{
    cupcake_timer_t *t;
    int i;

    t = &g_timers.named[CUPCAKE_TMR_START];
    if (t->active) {
        t->cfg.on_start = tmr_start_seq_on_start;
        t->cfg.on_tick = tmr_start_seq_on_tick;
    }

    t = &g_timers.named[CUPCAKE_TMR_GAME];
    if (t->active) {
        t->cfg.rate_fn = tmr_game_rate;
        t->cfg.on_tick = tmr_game_ot;
    }

    t = &g_timers.named[CUPCAKE_TMR_MISS];
    if (t->active) {
        if (t->cfg.rate_sec >= 0.49f && t->cfg.rate_sec <= 0.51f) {
            t->cfg.on_start = tmr_miss_cupcake_start;
            t->cfg.on_tick = tmr_miss_cupcake_ot;
            t->cfg.on_end = tmr_miss_cupcake_end;
        } else if (t->cfg.rate_sec >= 0.39f && t->cfg.rate_sec <= 0.41f) {
            t->cfg.on_tick = tmr_miss_couch_ot;
            t->cfg.on_end = tmr_miss_couch_end;
        }
    }

    t = &g_timers.named[CUPCAKE_TMR_PHASE];
    if (t->active) {
        t->cfg.on_start = tmr_phase_complete_start;
        t->cfg.on_end = tmr_phase_complete_end;
    }

    t = &g_timers.named[CUPCAKE_TMR_ACTION];
    if (t->active) {
        if (t->cfg.rate_sec >= 1.99f && t->cfg.rate_sec <= 2.01f) {
            t->cfg.on_start = tmr_sit_bonus_on_start;
            t->cfg.on_end = tmr_sit_bonus_end;
        } else {
            t->cfg.on_start = tmr_bart_action_on_start;
            t->cfg.on_end = tmr_bart_action_on_end;
        }
    }

    t = &g_timers.named[CUPCAKE_TMR_COUCH];
    if (t->active) {
        t->cfg.on_start = tmr_couch_spawn_on_start;
        t->cfg.on_tick = tmr_couch_spawn_ot;
        t->cfg.on_end = tmr_couch_spawn_end;
    }

    cupcake_scoreboard_timers_restore(&g.play, &g_timers);

    for (i = 0; i < CUPCAKE_TMR_SCHEDULE_MAX; i++) {
        cupcake_timer_t *s = &g_timers.schedule[i];
        if (!s->active)
            continue;
        if (s->cfg.rate_sec >= 1.74f && s->cfg.rate_sec <= 1.76f)
            s->cfg.on_start = sched_on_m;
        else if (s->cfg.rate_sec >= 1.99f && s->cfg.rate_sec <= 2.01f)
            s->cfg.on_start = sched_on_m;
        else if (s->cfg.rate_sec >= 0.74f && s->cfg.rate_sec <= 0.76f)
            s->cfg.on_start = sched_phase_restart_finish;
    }

    if (g.play.mode == CUPCAKE_MODE_PLAY && !g.play.enabled &&
        cupcake_timer_active(&g_timers, CUPCAKE_TMR_GAME))
        cupcake_timer_pause(&g_timers, CUPCAKE_TMR_GAME);
}

void cupcake_on_start(int phase, int show_level)
{
    cupcake_play_state_t *p = &g.play;
    cupcake_timer_config_t cfg;

    g_start_ctx.phase = phase;
    g_start_ctx.show_level = show_level ? 1 : 0;

    p->mode = CUPCAKE_MODE_START;
    p->enabled = 0;
    cupcake_set_threshold(10000);
    p->scoreboard.show_con = 0;

    if (show_level) {
        cupcake_scoreboard_set_level(p, (int)p->level);
    } else {
        cupcake_scoreboard_set_level(p, 0);
        cupcake_scoreboard_set_phase(p, phase > 0 ? phase : 1);
    }

    host_sfx("stop");
    cupcake_timers_stop(&g_timers);

    memset(&cfg, 0, sizeof cfg);
    cfg.rate_sec = 3.16f;
    cfg.max_ticks = 2;
    cfg.on_start = tmr_start_seq_on_start;
    cfg.on_tick = tmr_start_seq_on_tick;
    cupcake_timer_start(&g_timers, CUPCAKE_TMR_START, &cfg);

    CUPCAKE_DEV_LOG( "Start intro: level %u phase %d (show_level=%d)\n", p->level, phase,
            show_level);
}

void cupcake_set_callback(cupcake_cb_t cb)
{
    g_cb = cb;
}

void cupcake_set_buttons(uint16_t buttons)
{
    g_buttons_prev = g_buttons;
    g_buttons = buttons;
}

void cupcake_buttons_sync(uint16_t buttons)
{
    g_buttons_prev = buttons;
    g_buttons = buttons;
}

void cupcake_set_debug_pin(const char *name, int freeze_demo)
{
#if !defined(CUPCAKE_GNW)
    debug_pin_tune_count = 0;
    debug_pin_name[0] = '\0';
    debug_pin_freeze_demo = freeze_demo;
    if (name && name[0]) {
        snprintf(debug_pin_name, sizeof debug_pin_name, "%s", name);
        CUPCAKE_DEV_LOG("Debug pin: %s (freeze demo=%d)\n", debug_pin_name, freeze_demo);
    }
#else
    (void)name;
    (void)freeze_demo;
#endif
}

int cupcake_set_debug_pin_from_tune(int freeze_demo)
{
#if !defined(CUPCAKE_GNW)
    char path[512];

    debug_pin_name[0] = '\0';
    debug_pin_freeze_demo = freeze_demo;
    debug_pin_tune_count = load_lcd_tune_entries(debug_pin_tune, CUPCAKE_PIN_MAX);
    tune_file_path(path, sizeof path);
    CUPCAKE_DEV_LOG("Debug pin: %d sprite(s) from %s (freeze demo=%d)\n", debug_pin_tune_count,
            path, freeze_demo);
    return debug_pin_tune_count;
#else
    (void)freeze_demo;
    return 0;
#endif
}

void cupcake_set_debug_pin_solo(int solo)
{
#if !defined(CUPCAKE_GNW)
    debug_pin_solo = solo ? 1 : 0;
#else
    (void)solo;
#endif
}

void cupcake_init(void)
{
    cupcake_cb_t cb = g_cb;
    uint8_t record = g.play.record;

    memset(&g, 0, sizeof g);
    g_cb = cb;
    cupcake_play_state_reset(&g.play);
    g.play.record = record;
    cupcake_hiscore_load(&g.play);
    cupcake_play_state_start_demo(&g.play);
    cupcake_timers_init(&g_timers);
    cupcake_timers_set_post_update(cupcake_run_deferred_work);
    cupcake_rng_init();
    g.rng_state = cupcake_rng_get_state();
    {
        cupcake_scoreboard_host_t host = {
            .pause = cupcake_pause,
            .resume = cupcake_resume,
            .sfx = host_sfx_hook,
            .on_score_change = host_on_score_change,
        };
        cupcake_scoreboard_set_host(&host);
    }
    cupcake_set_threshold(10000);
    /* debug_pin_* / g_debug_start are host tuning state — do not clear here. */
#ifdef CUPCAKE_DEBUG_PIN_SPRITE_STR
    if (!debug_pin_name[0])
        cupcake_set_debug_pin(CUPCAKE_DEBUG_PIN_SPRITE_STR, 1);
#endif
}

void cupcake_update(void)
{
    g.frame++;
    cupcake_timers_update(&g_timers, CUPCAKE_TICK_DT);

    if (g.play.mode == CUPCAKE_MODE_DEMO && g.play.level == 0 &&
        !(debug_pin_is_active() && debug_pin_freeze_demo)) {
        g.demo_tick++;
        if (g.demo_tick >= CUPCAKE_DEMO_TICKS_PER_FRAME) {
            g.demo_tick = 0;
            g.demo_frame = (uint16_t)((g.demo_frame + 1) % CUPCAKE_DEMO_FRAME_COUNT);
        }
        g.play.scoreboard.value = cupcake_hiscore_attract(&g.play);
    }

    cupcake_handle_input();
}

void cupcake_draw(void)
{
    unsigned i;
    const char *name;

    refresh_lcd_tune_override();
    host_frame();

    if (g.play.mode == CUPCAKE_MODE_DEMO) {
        if (g.play.level == 0 && !(debug_pin_is_active() && debug_pin_solo)) {
            const cupcake_demo_frame_t *fr = &cupcake_demo_frames[g.demo_frame];
            for (i = 0; i < fr->count; i++) {
                name = cupcake_demo_sprite_name(fr->sprite_ids[i]);
                if (name)
                    draw_sprite_auto(name);
            }
        }
        if (!(debug_pin_is_active() && debug_pin_solo))
            cupcake_scoreboard_draw(&g.play, draw_scoreboard_sprite, NULL);
        draw_debug_pin();
        return;
    }

    if (g.play.mode == CUPCAKE_MODE_START || g.play.mode == CUPCAKE_MODE_PLAY ||
        g.play.mode == CUPCAKE_MODE_OVER) {
        /* Play always uses live game visibility + lcd_tune.txt coords. Pin-solo is demo-only. */
        draw_bart_play(&g.play);
        cupcake_scoreboard_draw(&g.play, draw_scoreboard_sprite, NULL);
        return;
    }

    draw_debug_pin();
}

size_t cupcake_state_size(void)
{
    return sizeof(cupcake_save_blob_t);
}

void cupcake_save_state(void *buf)
{
    cupcake_save_blob_t *blob = (cupcake_save_blob_t *)buf;

    if (!blob)
        return;
    blob->magic = CUPCAKE_SAVE_MAGIC;
    blob->version = CUPCAKE_SAVE_VERSION;
    g.rng_state = cupcake_rng_get_state();
    cupcake_timers_export(&g_timers, &g);
    blob->state = g;
}

int cupcake_load_state(const void *buf)
{
    const cupcake_save_blob_t *blob = (const cupcake_save_blob_t *)buf;

    if (!blob || blob->magic != CUPCAKE_SAVE_MAGIC || blob->version != CUPCAKE_SAVE_VERSION)
        return 0;
    g = blob->state;
    cupcake_rng_set_state(g.rng_state);
    cupcake_timers_import(&g_timers, &g);
    cupcake_hiscore_sync_from_play(&g.play);
    cupcake_hiscore_save();
    cupcake_timers_restore();
    return 1;
}

const cupcake_state_t *cupcake_get_state(void)
{
    return &g;
}

cupcake_play_state_t *cupcake_play_state(void)
{
    return &g.play;
}

cupcake_timers_t *cupcake_timers(void)
{
    return &g_timers;
}
