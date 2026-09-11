/*
 * Game & Watch host — GWHB load via firmware; firmware calls through gw_core_bridge.
 */
#include <odroid_system.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(HOST_BUILD)
#include <sys/stat.h>
#endif

#include "main.h"
#include "common.h"
#include "gw_lcd.h"
#include "gw_audio.h"
#include "appid.h"
#include "gw_malloc.h"
#include "odroid_overlay.h"

#include "cupcake.h"
#include "cupcake_port.h"
#include "cupcake_sprites.h"
#include "cupcake_sprite_lcd.h"
#include "cupcake_hiscore.h"
#include "host_draw.h"
#include "host_audio.h"
#include "cupcake_input.h"
#include "gnw_assets.h"
#include "cupcake_trace.h"
#include "main_cupcake.h"

#include "cupcake_data.h"

#ifndef HOST_BUILD
#include "gw_core_bridge.h"
#else
#include "host_compat.h"
#endif

#define CUPCAKE_FPS         30
#define CUPCAKE_SAMPLE_RATE 22050
#define FB_W                WIDTH
#define FB_H                HEIGHT

static uint16_t g_buttons;
static uint16_t *g_draw_fb;
static host_atlas_t g_host;
static host_bezel_t g_bezel;
static host_lcd_rect_t g_lcd_rect;
static int g_bezel_w;
static int g_bezel_h;

static bool SaveState(const char *path)
{
    void *st;

    if (!path)
        return false;
    st = ram_malloc(cupcake_state_size());
    if (!st)
        return false;
    cupcake_save_state(st);
    {
        FILE *fp = fopen(path, "wb");
        if (!fp)
            return false;
        fwrite(st, 1, cupcake_state_size(), fp);
        fclose(fp);
    }
    return true;
}

static bool LoadState(const char *path)
{
    void *st;

    if (!path)
        return false;
    st = ram_malloc(cupcake_state_size());
    if (!st)
        return false;
    {
        FILE *fp = fopen(path, "rb");
        if (!fp)
            return false;
        fread(st, 1, cupcake_state_size(), fp);
        fclose(fp);
    }
    cupcake_load_state(st);
    return true;
}

static int cupcake_cb_wrap(cupcake_cb_type_t type, const char *str_arg, int int_arg0,
                           int int_arg1)
{
    static uint32_t s_spr_log;

    switch (type) {
    case CUPCAKE_CB_FRAME:
        if (g_draw_fb)
            host_bezel_blit_rgb565(&g_bezel, g_draw_fb, FB_W, FB_H);
        return 0;
    case CUPCAKE_CB_SPR:
        if (s_spr_log < 24u) {
            const cupcake_sprite_rect_t *spr = str_arg ? cupcake_sprite_by_name(str_arg) : NULL;
            int log_x = int_arg0;
            int log_y = int_arg1;

            if ((log_x == CUPCAKE_LCD_AUTO || log_y == CUPCAKE_LCD_AUTO) && str_arg) {
                const cupcake_sprite_lcd_t *lcd = cupcake_sprite_lcd_by_name(str_arg);

                if (lcd) {
                    log_x = lcd->lcd_x;
                    log_y = cupcake_sprite_lcd_resolve_y(str_arg, lcd->lcd_y);
                }
            }

            if (spr) {
                cupcake_trace("spr[%2u] %-12s lcd=%4d,%4d atlas=%3d,%3d %3dx%3d fb~%dx%d",
                              (unsigned)s_spr_log, str_arg, log_x, log_y, spr->x, spr->y, spr->w,
                              spr->h,
                              (spr->w * FB_W + CUPCAKE_LCD_ATLAS_W - 1) / CUPCAKE_LCD_ATLAS_W,
                              (spr->h * FB_H + CUPCAKE_BEZEL_VISIBLE_H - 1) /
                                  CUPCAKE_BEZEL_VISIBLE_H);
            } else {
                cupcake_trace("spr[%2u] %-12s lcd=%4d,%4d (no atlas rect)",
                              (unsigned)s_spr_log, str_arg ? str_arg : "?", log_x, log_y);
            }
            s_spr_log++;
        }
        if (g_draw_fb)
            host_draw_sprite_rgb565_fb(&g_host, g_draw_fb, FB_W, FB_H, str_arg, int_arg0,
                                       int_arg1);
        return 0;
    case CUPCAKE_CB_BTN:
        return !!(g_buttons & (1u << int_arg0));
    case CUPCAKE_CB_SFX:
        if (str_arg && s_spr_log < 30u)
            cupcake_trace("sfx: %s", str_arg);
        if (str_arg)
            host_audio_play(str_arg);
        return 0;
    case CUPCAKE_CB_SOUND_TOGGLE:
        host_audio_toggle_mute();
        return 0;
    default:
        return 0;
    }
}

static void blit_frame(void)
{
    /* Menu / overlay repaint: redraw the game into the active LCD buffer. */
    g_draw_fb = (uint16_t *)lcd_get_active_buffer();
    if (g_draw_fb)
        cupcake_draw();
    common_ingame_overlay();
}

static void setup_hiscore_path(void)
{
    char path[512];

#if defined(HOST_BUILD)
    {
        int err = mkdir("host_saves", 0755);
        (void)err; /* EEXIST is fine */
    }
    snprintf(path, sizeof path, "host_saves/cupcake_hiscores.dat");
#else
    snprintf(path, sizeof path, "/data/homebrew/cupcake_hiscores.dat");
#endif
    cupcake_hiscore_set_path(path);
}

/*
 * common_emu_input_loop() can block inside retro-go menus. Refresh frame
 * timing, restart SAI DMA after firmware mute, and re-sync gamepad edges so a
 * PAUSE release from closing the menu does not look like an in-game button
 * release to cupcake_update().
 */
static void cupcake_resume_after_blocking_input(common_emu_state_t *emu, uint16_t audio_frames,
                                                odroid_gamepad_state_t *pad, uint32_t blocked_ms)
{
    if (!emu || blocked_ms < 40u)
        return;

    emu->last_sync_time = HAL_GetTick();
    audio_start_playing(audio_frames);

    if (pad) {
        odroid_input_read_gamepad(pad);
        /* Menu release of PAUSE must not look like an in-game edge. */
        if (pad->values[ODROID_INPUT_VOLUME])
            g_buttons = 0;
        else
            cupcake_input_from_odroid(pad, &g_buttons);
        cupcake_buttons_sync(g_buttons);
    }

    cupcake_trace("audio: resume after menu block (%lu ms)", (unsigned long)blocked_ms);
}

void app_main_cupcake(uint8_t load_state, uint8_t start_paused, int8_t save_slot)
{
    odroid_dialog_choice_t options[] = {ODROID_DIALOG_CHOICE_LAST};
    odroid_gamepad_state_t pad;
    int visible_bezel_h;
    int audio_frames;

    (void)save_slot;

    cupcake_trace_init();
    cupcake_trace("app_main: load_state=%u start_paused=%u save_slot=%d",
                  (unsigned)load_state, (unsigned)start_paused, (int)save_slot);

    if (gnw_assets_load(&g_host, &g_bezel, &g_bezel_w, &g_bezel_h) != 0) {
        cupcake_trace("fail: gnw_assets_load (embedded RGB565 missing or invalid)");
        odroid_overlay_alert("Cupcake: embedded assets failed.\nRebuild with assets/");
        return;
    }
    cupcake_trace("app_main: assets ok bezel %dx%d atlas %dx%d fb %dx%d",
                  g_bezel_w, g_bezel_h, g_host.atlas_w, g_host.atlas_h, FB_W, FB_H);
    cupcake_trace("embed: atlas_src %dx%d ram_est=%u embed_bytes=%u",
                  CUPCAKE_GNW_ATLAS_SOURCE_W, CUPCAKE_GNW_ATLAS_SOURCE_H,
                  (unsigned)CUPCAKE_EMBED_RAM_ESTIMATE, (unsigned)CUPCAKE_EMBED_BYTES);

    visible_bezel_h = CUPCAKE_BEZEL_VISIBLE_H;
    if (g_bezel_h < visible_bezel_h)
        visible_bezel_h = g_bezel_h;
    g_lcd_rect = host_lcd_rect_for_framebuffer(FB_W, FB_H, g_bezel_w, visible_bezel_h);
    cupcake_trace("lcd_rect: x=%d y=%d w=%d h=%d", g_lcd_rect.x, g_lcd_rect.y, g_lcd_rect.w,
                  g_lcd_rect.h);
    cupcake_trace("heap free after assets: %u bytes", (unsigned)ram_get_free_size());

    odroid_system_init(APPID_HOMEBREW, CUPCAKE_SAMPLE_RATE);
    odroid_system_emu_init(&LoadState, &SaveState, NULL, NULL, NULL, NULL, NULL);
    cupcake_trace("app_main: odroid_system_init ok");

    audio_frames = CUPCAKE_SAMPLE_RATE / CUPCAKE_FPS;
    audio_start_playing((uint16_t)audio_frames);

    if (start_paused)
        common_emu_state.pause_after_frames = 2;
    else
        common_emu_state.pause_after_frames = 0;
    common_emu_state.frame_time_10us = (uint16_t)(100000 / CUPCAKE_FPS + 0.5f);

    setup_hiscore_path();
    cupcake_init();
    cupcake_set_callback(cupcake_cb_wrap);
    cupcake_trace("app_main: cupcake_init ok");

    if (host_audio_init(gnw_assets_base()) != 0) {
        cupcake_trace("warn: host_audio_init failed (continuing muted)");
        odroid_overlay_alert("Cupcake: audio init failed.\nRebuild with assets/");
    } else {
        cupcake_trace("app_main: host_audio_init ok (ADPCM stream, rate=%d, pack_gain=%.2f)",
                      odroid_audio_sample_rate_get(), (double)CUPCAKE_GNW_PCM_PACK_GAIN);
    }

    if (load_state)
        odroid_system_emu_load_state(save_slot);
    else
        lcd_clear_buffers();

    cupcake_trace("app_main: entering main loop");

    while (true) {
        bool draw_frame;
        static uint32_t s_frame_log;
        static uint16_t s_last_buttons;
        uint32_t input_t0;
        uint32_t input_ms;

        wdog_refresh();
        draw_frame = common_emu_frame_loop();

        odroid_input_read_gamepad(&pad);

        /*
         * PAUSE/SET (ODROID_INPUT_VOLUME) is the retro-go modifier for volume /
         * brightness macros and the pause menu. Remember it before
         * common_emu_input_loop(), which may clear the pad when a macro fires.
         */
        {
            bool pause_modifier = pad.values[ODROID_INPUT_VOLUME] != 0;

            input_t0 = HAL_GetTick();
            common_emu_input_loop(&pad, options, &blit_frame);
            input_ms = HAL_GetTick() - input_t0;
            cupcake_resume_after_blocking_input(&common_emu_state, (uint16_t)audio_frames, &pad,
                                                input_ms);
            common_emu_input_loop_handle_turbo(&pad);

            /* Map after input_loop so consumed macro keys never reach the game. */
            if (pause_modifier)
                g_buttons = 0;
            else
                cupcake_input_from_odroid(&pad, &g_buttons);
        }

        if (s_frame_log < 5u) {
            cupcake_trace("frame %lu draw=%d buttons=0x%04x", (unsigned long)s_frame_log,
                          draw_frame ? 1 : 0, (unsigned)g_buttons);
            s_frame_log++;
        }

        if (g_buttons != s_last_buttons) {
            cupcake_trace("input: buttons 0x%04x -> 0x%04x", (unsigned)s_last_buttons,
                          (unsigned)g_buttons);
            s_last_buttons = g_buttons;
        }

        cupcake_set_buttons(g_buttons);

        cupcake_update();
        host_audio_pump(odroid_audio_sample_rate_get() / CUPCAKE_FPS);

        if (draw_frame) {
            g_draw_fb = (uint16_t *)lcd_get_active_buffer();
            cupcake_draw();
            common_ingame_overlay();
            lcd_swap();
        }
        common_emu_sound_sync(false);
    }
}
