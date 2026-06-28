/*
 * Game & Watch overlay host — loads via firmware homebrew slot; calls firmware through ABI.
 * Build: make cupcake-bin
 */
#include <odroid_system.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "common.h"
#include "gw_lcd.h"
#include "gw_audio.h"
#include "appid.h"
#include "gw_malloc.h"
#include "odroid_overlay.h"
#include "rg_abi.h"

#include "cupcake.h"
#include "cupcake_port.h"
#include "cupcake_hiscore.h"
#include "host_draw.h"
#include "host_audio.h"
#include "cupcake_input.h"
#include "gnw_assets.h"
#include "cupcake_trace.h"

#define CUPCAKE_FPS        30
#define CUPCAKE_SAMPLE_RATE 22050
#define FB_W               WIDTH
#define FB_H               HEIGHT

static uint16_t g_buttons;
static uint16_t g_fb_data[FB_W * FB_H];
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
        if (!fp) {
            return false;
        }
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
        if (!fp) {
            return false;
        }
        fread(st, 1, cupcake_state_size(), fp);
        fclose(fp);
    }
    cupcake_load_state(st);
    return true;
}

static int cupcake_cb_wrap(cupcake_cb_type_t type, const char *str_arg, int int_arg0,
                           int int_arg1)
{
    switch (type) {
    case CUPCAKE_CB_FRAME:
        host_bezel_blit_rgb565(&g_bezel, g_fb_data, FB_W, FB_H);
        host_clear_lcd_transparent(&g_host);
        return 0;
    case CUPCAKE_CB_SPR:
        host_draw_sprite(&g_host, str_arg, int_arg0, int_arg1);
        return 0;
    case CUPCAKE_CB_BTN:
        return !!(g_buttons & (1u << int_arg0));
    case CUPCAKE_CB_SFX:
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
    pixel_t *lcd = (pixel_t *)lcd_get_active_buffer();

    host_lcd_blit_rgb565(&g_host, &g_lcd_rect, g_fb_data, FB_W, FB_H);
    if (lcd)
        memcpy(lcd, g_fb_data, (size_t)FB_W * (size_t)FB_H * sizeof(uint16_t));
}

static void setup_hiscore_path(void)
{
    char path[512];

    snprintf(path, sizeof path, "/retro-go/saves/cupcake_hiscores.dat");
    cupcake_hiscore_set_path(path);
}

void app_main_cupcake(uint8_t load_state, uint8_t start_paused, int8_t save_slot)
{
    odroid_dialog_choice_t options[] = {ODROID_DIALOG_CHOICE_LAST};
    odroid_gamepad_state_t pad;
    common_emu_state_t *emu;
    int visible_bezel_h;
    int audio_frames;

    (void)save_slot;

    if (!gw_abi_ok()) {
        cupcake_trace("fail: firmware ABI mismatch (version/size)");
        odroid_overlay_alert("Cupcake: firmware ABI mismatch.\nFlash recent retro-go-sd.\nSee SD: retro-go/saves/cupcake_debug.log");
        return;
    }

    gw_abi_bind_stdio();
    cupcake_trace("app_main: ABI ok");

    emu = gw_common_emu_state();
    if (!emu) {
        cupcake_trace("fail: common_emu_state NULL");
        odroid_overlay_alert("Cupcake: firmware ABI incomplete.\nSee SD: retro-go/saves/cupcake_debug.log");
        return;
    }

    if (gnw_assets_load(&g_host, &g_bezel, &g_bezel_w, &g_bezel_h) != 0) {
        cupcake_trace("fail: gnw_assets_load");
#ifdef CUPCAKE_EMBEDDED_ASSETS
        odroid_overlay_alert("Cupcake: embedded assets failed.\nRebuild cupcake.bin with assets/\nSee SD: retro-go/saves/cupcake_debug.log");
#else
        odroid_overlay_alert("Cupcake: copy screen.jpg,\nsprites-color.png, audio/\nto /retro-go/cupcake/ on SD\nSee SD: retro-go/saves/cupcake_debug.log");
#endif
        return;
    }
    cupcake_trace("app_main: assets ok bezel %dx%d", g_bezel_w, g_bezel_h);

    visible_bezel_h = CUPCAKE_BEZEL_VISIBLE_H;
    if (g_bezel_h < visible_bezel_h)
        visible_bezel_h = g_bezel_h;
    g_lcd_rect = host_lcd_rect_for_framebuffer(FB_W, FB_H, g_bezel_w, visible_bezel_h);

    odroid_system_init(APPID_HOMEBREW, CUPCAKE_SAMPLE_RATE);
    odroid_system_emu_init(&LoadState, &SaveState, NULL, NULL, NULL, NULL);
    cupcake_trace("app_main: odroid_system_init ok");

    audio_frames = CUPCAKE_SAMPLE_RATE / CUPCAKE_FPS;
    audio_start_playing((uint16_t)audio_frames);

    if (start_paused)
        emu->pause_after_frames = 2;
    else
        emu->pause_after_frames = 0;
    emu->frame_time_10us = (uint16_t)(100000 / CUPCAKE_FPS + 0.5f);

    setup_hiscore_path();
    cupcake_init();
    cupcake_set_callback(cupcake_cb_wrap);
    cupcake_trace("app_main: cupcake_init ok");

    if (host_audio_init(gnw_assets_base()) != 0) {
        cupcake_trace("warn: host_audio_init failed (continuing muted)");
#ifdef CUPCAKE_EMBEDDED_ASSETS
        odroid_overlay_alert("Cupcake: embedded audio failed.\nRebuild cupcake.bin with assets/");
#else
        odroid_overlay_alert("Cupcake: audio disabled.\nCopy WAVs to /retro-go/cupcake/audio/");
#endif
    }

    if (load_state)
        odroid_system_emu_load_state(save_slot);
    else
        lcd_clear_buffers();

    cupcake_trace("app_main: entering main loop");

    while (true) {
        bool draw_frame;
        static uint32_t s_frame_log;

        wdog_refresh();
        draw_frame = common_emu_frame_loop();

        if (s_frame_log < 3u) {
            cupcake_trace("frame %lu", (unsigned long)s_frame_log);
            s_frame_log++;
        }

        odroid_input_read_gamepad(&pad);
        cupcake_input_from_odroid(&pad, &g_buttons);
        cupcake_set_buttons(g_buttons);

        common_emu_input_loop(&pad, options, &blit_frame);
        common_emu_input_loop_handle_turbo(&pad);

        cupcake_update();
        cupcake_draw();
        host_audio_pump(odroid_audio_sample_rate_get() / CUPCAKE_FPS);

        if (draw_frame) {
            blit_frame();
        }
        lcd_swap();
        common_emu_sound_sync(false);
    }
}
