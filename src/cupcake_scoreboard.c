/*
 * Scoreboard digit rendering — 7-segment digit{column}{segment} sprites (TASK-08+).
 */
#include "cupcake_scoreboard.h"

#include <stdio.h>
#include <string.h>

static const uint8_t segs_0[] = {0, 1, 3, 4, 5, 6};
static const uint8_t segs_1[] = {1, 3};
static const uint8_t segs_2[] = {0, 1, 2, 4, 5};
static const uint8_t segs_3[] = {0, 1, 2, 3, 4};
static const uint8_t segs_4[] = {1, 2, 3, 6};
static const uint8_t segs_5[] = {0, 2, 3, 4, 6};
static const uint8_t segs_6[] = {0, 2, 3, 4, 5, 6};
static const uint8_t segs_7[] = {0, 1, 3};
static const uint8_t segs_8[] = {0, 1, 2, 3, 4, 5, 6};
static const uint8_t segs_9[] = {0, 1, 2, 3, 4, 6};

static const uint8_t *const digit_segments[10] = {
    segs_0, segs_1, segs_2, segs_3, segs_4, segs_5, segs_6, segs_7, segs_8, segs_9,
};

static const uint8_t digit_segment_count[10] = {
    6, 2, 5, 5, 4, 5, 6, 4, 7, 6,
};

static const uint8_t segs_C[] = {0, 4, 5, 6};
static const uint8_t segs_O[] = {0, 1, 3, 4, 5, 6};
static const uint8_t segs_N[] = {0, 1, 3, 5, 6};
static const uint8_t segs_P[] = {0, 1, 2, 5, 6};
static const uint8_t segs_L[] = {4, 5, 6};
static const uint8_t segs_dash[] = {2};

static int char_segments(char ch, const uint8_t **segments, unsigned *count)
{
    if (ch >= '0' && ch <= '9') {
        *segments = digit_segments[ch - '0'];
        *count = digit_segment_count[ch - '0'];
        return 1;
    }
    switch (ch) {
    case 'C':
        *segments = segs_C;
        *count = 4;
        return 1;
    case 'O':
        *segments = segs_O;
        *count = 6;
        return 1;
    case 'N':
        *segments = segs_N;
        *count = 5;
        return 1;
    case 'P':
        *segments = segs_P;
        *count = 5;
        return 1;
    case 'L':
        *segments = segs_L;
        *count = 3;
        return 1;
    case '-':
        *segments = segs_dash;
        *count = 1;
        return 1;
    default:
        return 0;
    }
}

static void draw_char_at_column(int column, char ch, cupcake_scoreboard_draw_fn draw, void *ctx)
{
    const uint8_t *segments;
    unsigned count;
    unsigned i;
    char name[16];

    if (!draw || !char_segments(ch, &segments, &count))
        return;

    for (i = 0; i < count; i++) {
        cupcake_scoreboard_digit_sprite(column, segments[i], name, sizeof name);
        draw(name, ctx);
    }
}

static void format_display_value(uint32_t value, int alwayson, char *out, size_t out_sz)
{
    char tmp[16];
    int len;
    int pad;
    int i;

    snprintf(tmp, sizeof tmp, "%u", value);
    len = (int)strlen(tmp);
    pad = 0;
    if (alwayson > 0 && len < alwayson)
        pad = alwayson - len;
    if (pad + len >= (int)out_sz)
        pad = (int)out_sz - len - 1;
    if (pad < 0)
        pad = 0;
    for (i = 0; i < pad; i++)
        out[i] = '0';
    snprintf(out + pad, out_sz - (size_t)pad, "%s", tmp);
}

int cupcake_scoreboard_slot_digit(uint32_t value, int alwayson, int num_digits,
                                  int slot_from_right)
{
    char text[16];
    int idx;

    if (slot_from_right < 0 || slot_from_right >= num_digits)
        return -1;

    format_display_value(value, alwayson, text, sizeof text);
    idx = (int)strlen(text) - 1 - slot_from_right;
    if (idx < 0)
        return -1;
    if (text[idx] < '0' || text[idx] > '9')
        return -1;
    return text[idx] - '0';
}

void cupcake_scoreboard_digit_sprite(int column, int segment, char *buf, size_t bufsz)
{
    if (!buf || bufsz == 0)
        return;
    snprintf(buf, bufsz, "digit%d%d", column, segment);
}

void cupcake_scoreboard_draw_slot(int column, int digit, cupcake_scoreboard_draw_fn draw,
                                  void *ctx)
{
    const uint8_t *segments;
    unsigned count;

    if (!draw || digit < 0 || digit > 9)
        return;

    segments = digit_segments[digit];
    count = digit_segment_count[digit];
    cupcake_scoreboard_draw_segments(column, segments, count, draw, ctx);
}

void cupcake_scoreboard_draw_segments(int column, const uint8_t *segments, unsigned count,
                                      cupcake_scoreboard_draw_fn draw, void *ctx)
{
    char name[16];
    unsigned i;

    if (!draw || !segments)
        return;

    for (i = 0; i < count; i++) {
        cupcake_scoreboard_digit_sprite(column, segments[i], name, sizeof name);
        draw(name, ctx);
    }
}

void cupcake_scoreboard_draw_digits(uint32_t value, int alwayson, int num_digits,
                                    cupcake_scoreboard_draw_fn draw, void *ctx)
{
    int slot;

    if (!draw || num_digits <= 0)
        return;

    for (slot = 0; slot < num_digits; slot++) {
        int digit = cupcake_scoreboard_slot_digit(value, alwayson, num_digits, slot);
        if (digit < 0)
            continue;
        cupcake_scoreboard_draw_slot(slot, digit, draw, ctx);
    }
}

void cupcake_scoreboard_draw_text(const char *text, cupcake_scoreboard_draw_fn draw, void *ctx)
{
    size_t len;
    int col;

    if (!draw || !text)
        return;

    len = strlen(text);
    if (len > 3)
        len = 3;

    for (col = 0; col < (int)len; col++)
        draw_char_at_column(4 - col, text[col], draw, ctx);
}

void cupcake_scoreboard_set_phase(cupcake_play_state_t *play, int phase)
{
    if (!play)
        return;
    play->scoreboard.phase = (uint8_t)cupcake_phase_clamp(phase);
    play->scoreboard.disp = CUPCAKE_SB_DISP_PHASE;
}

void cupcake_scoreboard_set_level(cupcake_play_state_t *play, int level)
{
    if (!play)
        return;
    if (level < 0)
        level = 0;
    if (level > CUPCAKE_LEVEL_MAX)
        level = CUPCAKE_LEVEL_MAX;
    play->scoreboard.level = (uint8_t)level;
    if (level >= 1)
        play->scoreboard.disp = CUPCAKE_SB_DISP_LEVEL;
}

void cupcake_scoreboard_show_value(cupcake_play_state_t *play, uint32_t value)
{
    if (!play)
        return;
    play->scoreboard.value = value;
    play->scoreboard.disp = CUPCAKE_SB_DISP_VALUE;
}

void cupcake_scoreboard_show_run_score(cupcake_play_state_t *play)
{
    if (!play)
        return;
    play->scoreboard.disp = CUPCAKE_SB_DISP_SCORE;
}

static void draw_level_overlay(int level, cupcake_scoreboard_draw_fn draw, void *ctx)
{
    char label[8];

    if (!draw || level < 1 || level > CUPCAKE_LEVEL_MAX)
        return;
    snprintf(label, sizeof label, "L-%u", (unsigned)level);
    cupcake_scoreboard_draw_text(label, draw, ctx);
}

static void draw_overlay(const cupcake_scoreboard_state_t *sb, cupcake_scoreboard_draw_fn draw,
                         void *ctx)
{
    char label[8];

    if (!sb || !draw)
        return;

    if (sb->show_con) {
        cupcake_scoreboard_draw_text("CON", draw, ctx);
        return;
    }

    if (sb->disp == CUPCAKE_SB_DISP_LEVEL && sb->level >= 1 && sb->level <= CUPCAKE_LEVEL_MAX) {
        draw_level_overlay((int)sb->level, draw, ctx);
    } else if (sb->disp == CUPCAKE_SB_DISP_PHASE && sb->phase >= 1 &&
               sb->phase <= CUPCAKE_PHASE_MAX) {
        snprintf(label, sizeof label, "P-%u", (unsigned)sb->phase);
        cupcake_scoreboard_draw_text(label, draw, ctx);
    }
}

void cupcake_scoreboard_draw(const cupcake_play_state_t *play, cupcake_scoreboard_draw_fn draw,
                             void *ctx)
{
    const cupcake_scoreboard_state_t *sb;

    if (!play || !draw)
        return;

    sb = &play->scoreboard;

    switch (play->mode) {
    case CUPCAKE_MODE_DEMO:
        if (sb->show_con) {
            cupcake_scoreboard_draw_digits(sb->value, 2, 5, draw, ctx);
            cupcake_scoreboard_draw_text("CON", draw, ctx);
        } else if (sb->disp == CUPCAKE_SB_DISP_LEVEL) {
            draw_level_overlay((int)sb->level, draw, ctx);
        } else {
            cupcake_scoreboard_draw_digits(sb->value, 2, 5, draw, ctx);
        }
        break;
    case CUPCAKE_MODE_START:
    case CUPCAKE_MODE_PLAY:
    case CUPCAKE_MODE_OVER:
        if (sb->show_con) {
            cupcake_scoreboard_draw_digits(sb->score, 2, 5, draw, ctx);
            cupcake_scoreboard_draw_text("CON", draw, ctx);
        } else if (sb->disp == CUPCAKE_SB_DISP_LEVEL) {
            draw_level_overlay((int)sb->level, draw, ctx);
        } else if (sb->disp == CUPCAKE_SB_DISP_PHASE) {
            draw_overlay(sb, draw, ctx);
        } else {
            cupcake_scoreboard_draw_digits(sb->score, 2, 5, draw, ctx);
        }
        break;
    }
}

void cupcake_scoreboard_draw_demo_level(const cupcake_play_state_t *play,
                                        cupcake_scoreboard_draw_fn draw, void *ctx)
{
    if (!play || !draw)
        return;
    if (play->mode != CUPCAKE_MODE_DEMO || play->scoreboard.show_con)
        return;
    if (play->scoreboard.disp != CUPCAKE_SB_DISP_LEVEL)
        return;

    draw_level_overlay((int)play->scoreboard.level, draw, ctx);
}

void cupcake_scoreboard_draw_demo_hiscore(const cupcake_play_state_t *play,
                                          cupcake_scoreboard_draw_fn draw, void *ctx)
{
    if (!play || !draw)
        return;
    if (play->mode != CUPCAKE_MODE_DEMO || play->scoreboard.show_con)
        return;
    if (play->scoreboard.disp != CUPCAKE_SB_DISP_VALUE)
        return;

    cupcake_scoreboard_draw_digits(play->scoreboard.value, 2, 5, draw, ctx);
}

static cupcake_scoreboard_host_t g_scoreboard_host;
static cupcake_play_state_t *g_bonus_play;
static cupcake_scoreboard_bonus_cfg_t g_bonus_cfg;

static void bonus_apply_tick(cupcake_play_state_t *play, const cupcake_scoreboard_bonus_cfg_t *cfg,
                             int tick)
{
    int inc;

    if (!play || !cfg || cfg->increment <= 0)
        return;

    inc = cfg->increment;
    if (tick == 1 && cfg->play_five_on_first && g_scoreboard_host.sfx)
        g_scoreboard_host.sfx("five");

    play->points += (uint32_t)inc;
    play->scoreboard.score = play->points;
    cupcake_scoreboard_show_run_score(play);

    if (cfg->play_points_sfx && g_scoreboard_host.sfx)
        g_scoreboard_host.sfx("points");

    if (g_scoreboard_host.on_score_change)
        g_scoreboard_host.on_score_change();

    if (cfg->on_tick)
        cfg->on_tick(cfg->user_ctx, tick);
}

static void tmr_scoreboard_bonus_ot(void *ctx, int tick)
{
    cupcake_play_state_t *play = g_bonus_play;
    const cupcake_scoreboard_bonus_cfg_t *cfg = &g_bonus_cfg;

    (void)ctx;

    if (!play)
        return;

    if (play->scoreboard.bonus_points_left > 0)
        play->scoreboard.bonus_points_left--;

    bonus_apply_tick(play, cfg, tick);
}

static void tmr_scoreboard_bonus_end(void *ctx, int tick)
{
    cupcake_play_state_t *play = g_bonus_play;
    const cupcake_scoreboard_bonus_cfg_t *cfg = &g_bonus_cfg;

    (void)ctx;
    (void)tick;

    if (play)
        play->scoreboard.bonus_active = 0;

    if (cfg->on_end)
        cfg->on_end(cfg->user_ctx);

    if (g_scoreboard_host.resume)
        g_scoreboard_host.resume();
}

void cupcake_scoreboard_set_host(const cupcake_scoreboard_host_t *host)
{
    if (host)
        g_scoreboard_host = *host;
    else
        memset(&g_scoreboard_host, 0, sizeof g_scoreboard_host);
}

int cupcake_scoreboard_add_bonus_ex(cupcake_play_state_t *play, cupcake_timers_t *timers,
                                    const cupcake_scoreboard_bonus_cfg_t *cfg)
{
    cupcake_timer_config_t tcfg;

    if (!play || !timers || !cfg || cfg->point_ticks <= 0 || cfg->rate_sec <= 0.f)
        return 0;

    if (g_scoreboard_host.pause)
        g_scoreboard_host.pause();

    g_bonus_play = play;
    g_bonus_cfg = *cfg;

    play->scoreboard.bonus_active = 1;
    play->scoreboard.bonus_rate = cfg->rate_sec;
    play->scoreboard.bonus_points_left = (uint16_t)cfg->point_ticks;
    play->scoreboard.bonus_increment = (uint16_t)cfg->increment;

    memset(&tcfg, 0, sizeof tcfg);
    tcfg.rate_sec = cfg->rate_sec;
    tcfg.max_ticks = cfg->point_ticks;
    tcfg.on_tick = tmr_scoreboard_bonus_ot;
    tcfg.on_end = tmr_scoreboard_bonus_end;
    cupcake_timer_start(timers, CUPCAKE_TMR_BONUS, &tcfg);
    return 1;
}

void cupcake_scoreboard_timers_restore(cupcake_play_state_t *play, cupcake_timers_t *timers)
{
    cupcake_timer_t *t;

    if (!play || !timers)
        return;

    g_bonus_play = play;

    t = &timers->named[CUPCAKE_TMR_BONUS];
    if (t->active) {
        t->cfg.on_tick = tmr_scoreboard_bonus_ot;
        t->cfg.on_end = tmr_scoreboard_bonus_end;
    }
}
