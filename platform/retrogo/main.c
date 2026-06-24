/*
 * retro-go / G&W linux emulator host (same core as platform/sdl/main.c).
 * Build from game-and-watch-retro-go-sd/linux with Makefile.cupcake.
 */
#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <odroid_system.h>

#include "stb_image.h"

#include "cupcake.h"
#include "cupcake_port.h"
#include "host_draw.h"
#include "host_audio.h"
#include "cupcake_input.h"

#ifndef LINUX_EMU
#include "odroid_input.h"
#endif

#define WIN_W  320
#define WIN_H  240
#define LCD_W  CUPCAKE_LCD_ATLAS_W
#define LCD_H  CUPCAKE_LCD_ATLAS_H
#define APP_ID 31

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *fb_texture;
static uint16_t fb_data[WIN_W * WIN_H];
static uint8_t *bezel_pixels;
static int bezel_w, bezel_h;
static host_bezel_t bezel;
static host_lcd_rect_t lcd_rect;
static uint8_t *atlas_pixels;
static uint8_t *lcd_pixels;
static int atlas_w, atlas_h;
static uint16_t buttons;
static int run_loop = 1;

static host_atlas_t host;

static const char *asset_path(const char *name)
{
    static char buf[512];
    const char *base = getenv("CUPCAKE_ASSETS");
    if (!base || !base[0])
        base = "/home/odroid/cupcake";
    snprintf(buf, sizeof buf, "%s/%s", base, name);
    return buf;
}

static uint8_t *load_image_rgba(const char *path, int *w, int *h)
{
    int comp;
    return stbi_load(path, w, h, &comp, 4);
}

static const char *assets_base_dir(void)
{
    const char *base = getenv("CUPCAKE_ASSETS");
    if (!base || !base[0])
        base = "/home/odroid/cupcake";
    return base;
}

static int cupcake_cb_wrap(cupcake_cb_type_t type, const char *str_arg, int int_arg0,
                           int int_arg1)
{
    switch (type) {
    case CUPCAKE_CB_FRAME:
        host_bezel_blit_rgb565(&bezel, fb_data, WIN_W, WIN_H);
        host_clear_lcd_transparent(&host);
        return 0;
    case CUPCAKE_CB_SPR:
        host_draw_sprite(&host, str_arg, int_arg0, int_arg1);
        return 0;
    case CUPCAKE_CB_BTN:
        return !!(buttons & (1u << int_arg0));
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

static bool SaveState(const char *path)
{
    void *st = malloc(cupcake_state_size());
    cupcake_save_state(st);
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        free(st);
        return false;
    }
    fwrite(st, 1, cupcake_state_size(), fp);
    fclose(fp);
    free(st);
    return true;
}

static bool LoadState(const char *path)
{
    void *st = malloc(cupcake_state_size());
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        free(st);
        return false;
    }
    fread(st, 1, cupcake_state_size(), fp);
    fclose(fp);
    cupcake_load_state(st);
    free(st);
    return true;
}

static void input_read_gamepad(void)
{
    SDL_Event ev;

#ifdef LINUX_EMU
    cupcake_input_from_sdl_keyboard(SDL_GetKeyboardState(NULL), &buttons);
#else
    odroid_gamepad_state_t pad;

    odroid_input_read_gamepad(&pad);
    cupcake_input_from_odroid(&pad, &buttons);
#endif

    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
            run_loop = 0;
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F3)
            SaveState("./cupcake.sav");
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F4)
            LoadState("./cupcake.sav");
    }
}

static void present_frame(void)
{
    host_lcd_blit_rgb565(&host, &lcd_rect, fb_data, WIN_W, WIN_H);
    SDL_UpdateTexture(fb_texture, NULL, fb_data, WIN_W * (int)sizeof(uint16_t));
    SDL_RenderCopy(renderer, fb_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
    int visible_bezel_h;

    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        return 1;

    window = SDL_CreateWindow("cupcake", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, WIN_W, WIN_H, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    fb_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                                   SDL_TEXTUREACCESS_STREAMING, WIN_W, WIN_H);

    bezel_pixels = load_image_rgba(asset_path("screen.jpg"), &bezel_w, &bezel_h);
    atlas_pixels = load_image_rgba(asset_path("sprites-draw.png"), &atlas_w, &atlas_h);
    if (!atlas_pixels)
        atlas_pixels = load_image_rgba(asset_path("sprites-color.png"), &atlas_w, &atlas_h);

    if (!bezel_pixels || !atlas_pixels) {
        fprintf(stderr,
                "Missing assets in %s (need screen.jpg and sprites-color.png)\n",
                asset_path(""));
        return 1;
    }

    visible_bezel_h = CUPCAKE_BEZEL_VISIBLE_H;
    if (bezel_h < visible_bezel_h)
        visible_bezel_h = bezel_h;

    bezel.pixels = bezel_pixels;
    bezel.w = bezel_w;
    bezel.h = bezel_h;
    bezel.visible_h = visible_bezel_h;
    lcd_rect = host_lcd_rect_for_framebuffer(WIN_W, WIN_H, bezel_w, visible_bezel_h);

    lcd_pixels = calloc(LCD_W * LCD_H, 4);

    host.atlas = atlas_pixels;
    host.atlas_w = atlas_w;
    host.atlas_h = atlas_h;
    host.atlas_stride = atlas_w * 4;
    host.lcd_pixels = lcd_pixels;
    host.lcd_w = LCD_W;
    host.lcd_h = LCD_H;
    host.lcd_stride = LCD_W * 4;

    fprintf(stderr, "retro-go host: bezel %dx%d (visible %d) -> %dx%d LCD rect %d,%d %dx%d\n",
            bezel_w, bezel_h, visible_bezel_h, WIN_W, WIN_H, lcd_rect.x, lcd_rect.y, lcd_rect.w,
            lcd_rect.h);

    odroid_system_init(APP_ID, 22050);
    odroid_system_emu_init(&LoadState, &SaveState, NULL, NULL);

    {
        char hiscore_path[512];
        const char *base = getenv("CUPCAKE_ASSETS");
        if (!base || !base[0])
            base = "/home/odroid/cupcake";
        snprintf(hiscore_path, sizeof hiscore_path, "%s/cupcake_hiscores.dat", base);
        cupcake_hiscore_set_path(hiscore_path);
    }

    cupcake_init();
    cupcake_set_callback(cupcake_cb_wrap);

    if (host_audio_init(assets_base_dir()) != 0)
        fprintf(stderr, "Warning: SFX disabled — copy assets/audio/ to %s/audio\n",
                assets_base_dir());

    while (run_loop) {
        input_read_gamepad();
        cupcake_set_buttons(buttons);
        cupcake_update();
        cupcake_draw();
        present_frame();
#if defined(CUPCAKE_AUDIO_ODROID) && !defined(LINUX_EMU)
        host_audio_pump(odroid_audio_sample_rate_get() / 30);
#endif
        SDL_Delay(33);
    }

    host_audio_shutdown();

    stbi_image_free(bezel_pixels);
    stbi_image_free(atlas_pixels);
    free(lcd_pixels);
    SDL_DestroyTexture(fb_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
