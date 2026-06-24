/*
 * PC / SDL2 host for Cupcake Crisis — test before retro-go deployment.
 */
#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_image.h"

#include "cupcake.h"
#include "cupcake_port.h"
#include "cupcake_sprite_lcd.h"
#include "cupcake_input.h"
#include "host_draw.h"
#include "host_audio.h"
#include "sprite_blit.h"

static float win_scale = 0.5f;
static int debug_draw;
static int show_lcd_border;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *bezel_tex;
static SDL_Texture *atlas_tex;
static SDL_Texture *lcd_tex;
static uint8_t *atlas_pixels;
static uint8_t *lcd_pixels;
static const char *atlas_file_loaded = "sprites-color.png";
static int bezel_w, bezel_h;
static int win_w, win_h;
static int atlas_w, atlas_h;
static SDL_Rect lcd_on_screen;
static uint16_t buttons;
static int run_loop = 1;

/* Applied after cupcake_init() so init does not wipe --pin. */
static const char *pin_sprite_arg;
static int pin_tune_file_arg;
static int pin_solo_arg;

static int parse_debug_start_spec(const char *spec)
{
    int level = 0;
    int phase = 0;
    unsigned score = 0;
    char sep = '\0';

    if (!spec || !spec[0])
        return 0;
    if (sscanf(spec, "%d%c%d%c%u", &level, &sep, &phase, &sep, &score) != 5)
        return 0;
    if (sep != ',' && sep != ':')
        return 0;
    cupcake_set_debug_start(level, phase, (uint32_t)score);
    return 1;
}

static void parse_args(int argc, char **argv)
{
    const char *env = getenv("CUPCAKE_WINDOW_SCALE");
    if (env && env[0])
        win_scale = (float)atof(env);

    pin_sprite_arg = NULL;
    pin_tune_file_arg = 0;
    pin_solo_arg = 0;
    show_lcd_border = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--debug")) {
            debug_draw = 1;
            show_lcd_border = 1;
        } else if (!strcmp(argv[i], "--lcd-border"))
            show_lcd_border = 1;
        else if (!strcmp(argv[i], "--pin")) {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                const char *arg = argv[++i];
                if (!strcmp(arg, "off") || !strcmp(arg, "none"))
                    pin_sprite_arg = "";
                else
                    pin_sprite_arg = arg;
            } else
                pin_tune_file_arg = 1;
        } else if (!strcmp(argv[i], "--pin-solo"))
            pin_solo_arg = 1;
        else if (!strcmp(argv[i], "--align")) {
            pin_tune_file_arg = 1;
            pin_solo_arg = 1;
            show_lcd_border = 1;
        } else if (!strcmp(argv[i], "--scale") && i + 1 < argc)
            win_scale = (float)atof(argv[++i]);
        else if (strncmp(argv[i], "--scale=", 8) == 0)
            win_scale = (float)atof(argv[i] + 8);
        else if (!strcmp(argv[i], "--start") && i + 1 < argc) {
            if (!parse_debug_start_spec(argv[++i]))
                fprintf(stderr, "Warning: --start needs LEVEL,PHASE,SCORE (e.g. 1,1,9900)\n");
        } else if (strncmp(argv[i], "--start=", 8) == 0) {
            if (!parse_debug_start_spec(argv[i] + 8))
                fprintf(stderr, "Warning: --start= needs LEVEL,PHASE,SCORE (e.g. 1,1,9900)\n");
        }
    }

    if (win_scale < 0.2f)
        win_scale = 0.2f;
    if (win_scale > 2.0f)
        win_scale = 2.0f;

    if (!cupcake_debug_start_pending()) {
        const char *env_start = getenv("CUPCAKE_DEBUG_START");
        if (env_start && env_start[0] && !parse_debug_start_spec(env_start))
            fprintf(stderr, "Warning: CUPCAKE_DEBUG_START needs LEVEL,PHASE,SCORE\n");
    }

    /* Alignment: show full 1024x800 sprite buffer bounds on the bezel. */
    if (pin_tune_file_arg || (pin_sprite_arg && pin_sprite_arg[0]))
        show_lcd_border = 1;
}

static const char *asset_path(const char *name)
{
    static char buf[512];
    const char *base = getenv("CUPCAKE_ASSETS");
    if (!base || !base[0])
        base = "assets";
    snprintf(buf, sizeof buf, "%s/%s", base, name);
    return buf;
}

static const char *assets_base_dir(void)
{
    const char *base = getenv("CUPCAKE_ASSETS");
    if (!base || !base[0])
        base = "assets";
    return base;
}

static uint8_t *load_image_rgba(const char *path, int *w, int *h)
{
    int comp;
    return stbi_load(path, w, h, &comp, 4);
}

/* SDL expects the right channel order — direct UpdateTexture often fails on Windows. */
static SDL_Texture *texture_from_rgba(SDL_Renderer *ren, uint8_t *px, int w, int h)
{
    SDL_Surface *surf =
        SDL_CreateRGBSurfaceWithFormatFrom(px, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
    if (!surf) {
        fprintf(stderr, "Surface failed: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    if (!tex)
        fprintf(stderr, "Texture failed: %s\n", SDL_GetError());
    else
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}

static void update_lcd_screen_rect(void)
{
    float sx = (float)win_w / (float)bezel_w;
    float sy = (float)win_h / (float)bezel_h;

    lcd_on_screen.x = (int)(CUPCAKE_LCD_ATLAS_X * sx + 0.5f);
    lcd_on_screen.y = (int)(CUPCAKE_LCD_ATLAS_Y * sy + 0.5f);
    lcd_on_screen.w = (int)(CUPCAKE_LCD_ATLAS_W * sx + 0.5f);
    lcd_on_screen.h = (int)(CUPCAKE_LCD_ATLAS_H * sy + 0.5f);
}

static void lcd_clear_frame(void)
{
    if (lcd_pixels)
        memset(lcd_pixels, 0, (size_t)CUPCAKE_LCD_W * CUPCAKE_LCD_H * 4);
}

static int draw_sprite_sdl(const char *name, int lcd_x, int lcd_y)
{
    const cupcake_sprite_rect_t *spr = cupcake_sprite_by_name(name);
    const cupcake_sprite_lcd_t *lcd;
    if (!spr || !atlas_pixels || !lcd_pixels)
        return 0;
    if (!host_sprite_rect_ok(name)) {
        fprintf(stderr, "sprite skipped (bad rect): %s %dx%d\n", name, spr->w, spr->h);
        return 0;
    }

    if (lcd_x == CUPCAKE_LCD_AUTO || lcd_y == CUPCAKE_LCD_AUTO) {
        lcd = cupcake_sprite_lcd_by_name(name);
        if (!lcd)
            return 0;
        lcd_x = lcd->lcd_x;
        lcd_y = cupcake_sprite_lcd_resolve_y(name, lcd->lcd_y);
    }

    sprite_blit_masked(atlas_pixels, atlas_w * 4, spr, name, lcd_pixels, CUPCAKE_LCD_W * 4,
                       CUPCAKE_LCD_W, CUPCAKE_LCD_H, lcd_x, lcd_y);
    if (debug_draw)
        fprintf(stderr, "drew %s @ lcd %d,%d (%dx%d)\n", name, lcd_x, lcd_y, spr->w, spr->h);
    return 1;
}

static int cupcake_cb_wrap(cupcake_cb_type_t type, const char *str_arg, int int_arg0,
                           int int_arg1)
{
    switch (type) {
    case CUPCAKE_CB_FRAME:
        return 0;
    case CUPCAKE_CB_SPR:
        return draw_sprite_sdl(str_arg, int_arg0, int_arg1);
    case CUPCAKE_CB_BTN:
        return !!(buttons & (1u << int_arg0));
    case CUPCAKE_CB_SFX:
        if (str_arg) {
            if (debug_draw)
                fprintf(stderr, "[sfx] %s\n", str_arg);
            host_audio_play(str_arg);
        }
        return 0;
    default:
        return 0;
    }
}

static void draw_lcd_border(void)
{
    /* 3px outline — marks the 1024x800 logical LCD (full visible screen.jpg). */
    SDL_SetRenderDrawColor(renderer, 0, 255, 96, 255);
    for (int i = 0; i < 3; i++) {
        SDL_Rect box = {
            lcd_on_screen.x - i,
            lcd_on_screen.y - i,
            lcd_on_screen.w + 2 * i,
            lcd_on_screen.h + 2 * i,
        };
        SDL_RenderDrawRect(renderer, &box);
    }
}

static void draw_sanity_sprite(void)
{
    const cupcake_sprite_rect_t *spr = cupcake_sprite_by_name("cake04");
    if (!spr || !atlas_tex)
        return;

    SDL_Rect src = {spr->x, spr->y, spr->w, spr->h};
    int dw = (int)(spr->w * (lcd_on_screen.w / (float)CUPCAKE_LCD_ATLAS_W) + 0.5f);
    int dh = (int)(spr->h * (lcd_on_screen.h / (float)CUPCAKE_LCD_ATLAS_H) + 0.5f);
    if (dw < 24)
        dw = 24;
    if (dh < 24)
        dh = 24;
    SDL_Rect dst = {
        lcd_on_screen.x + lcd_on_screen.w / 2 - dw / 2,
        lcd_on_screen.y + lcd_on_screen.h / 2 - dh / 2,
        dw,
        dh,
    };
    SDL_RenderCopy(renderer, atlas_tex, &src, &dst);
}

static void input_poll(void)
{
    SDL_Event ev;

    cupcake_input_from_sdl_keyboard(SDL_GetKeyboardState(NULL), &buttons);
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT)
            run_loop = 0;
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
            run_loop = 0;
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_x)
            fprintf(stderr, "Key X (Select) pressed\n");
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_z)
            fprintf(stderr, "Key Z (Action) pressed\n");
        if (ev.type == SDL_KEYDOWN &&
            (ev.key.keysym.sym == SDLK_LEFT || ev.key.keysym.sym == SDLK_RIGHT ||
             ev.key.keysym.sym == SDLK_UP || ev.key.keysym.sym == SDLK_DOWN))
            fprintf(stderr, "Arrow key pressed\n");
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F3) {
            void *st = malloc(cupcake_state_size());
            cupcake_save_state(st);
            FILE *fp = fopen("cupcake.sav", "wb");
            if (fp) {
                fwrite(st, 1, cupcake_state_size(), fp);
                fclose(fp);
            }
            free(st);
        }
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F4) {
            void *st = malloc(cupcake_state_size());
            FILE *fp = fopen("cupcake.sav", "rb");
            if (fp) {
                fread(st, 1, cupcake_state_size(), fp);
                fclose(fp);
                cupcake_load_state(st);
            }
            free(st);
        }
    }
}

static void present(void)
{
    SDL_SetRenderDrawColor(renderer, 40, 40, 48, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, bezel_tex, NULL, NULL);

    lcd_clear_frame();
    cupcake_set_buttons(buttons);
    cupcake_update();
    cupcake_draw();

    if (lcd_tex && lcd_pixels) {
        SDL_UpdateTexture(lcd_tex, NULL, lcd_pixels, CUPCAKE_LCD_W * 4);
        SDL_RenderCopy(renderer, lcd_tex, NULL, &lcd_on_screen);
    }

    if (debug_draw)
        draw_sanity_sprite();

    if (show_lcd_border)
        draw_lcd_border();

    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
    int loaded_bezel_h;

    parse_args(argc, argv);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    uint8_t *bezel_px = load_image_rgba(asset_path("screen.jpg"), &bezel_w, &loaded_bezel_h);
    atlas_pixels = load_image_rgba(asset_path(atlas_file_loaded), &atlas_w, &atlas_h);

    if (!bezel_px || !atlas_pixels) {
        fprintf(stderr,
                "Missing assets in %s (need screen.jpg and sprites-color.png; run: make gen)\n",
                asset_path(""));
        return 1;
    }

    {
        fprintf(stderr, "Atlas: %s (%dx%d)\n", atlas_file_loaded, atlas_w, atlas_h);
        const cupcake_sprite_rect_t *c4 = cupcake_sprite_by_name("cake04");
        const cupcake_sprite_rect_t *c43 = cupcake_sprite_by_name("cake43");
        if (c4)
            fprintf(stderr, "  cake04 (sanity) @ %d,%d %dx%d\n", c4->x, c4->y, c4->w, c4->h);
        if (c43)
            fprintf(stderr,
                    "  cake43 @ %d,%d %dx%d (atlas tex = digits, not cupcakes)\n",
                    c43->x, c43->y, c43->w, c43->h);
    }

    bezel_h = CUPCAKE_BEZEL_VISIBLE_H;
    if (loaded_bezel_h < bezel_h)
        bezel_h = loaded_bezel_h;

    win_w = (int)(bezel_w * win_scale + 0.5f);
    win_h = (int)(bezel_h * win_scale + 0.5f);
    update_lcd_screen_rect();

    {
        char title[64];
        snprintf(title, sizeof title, "Cupcake Crisis (%.0f%%)", win_scale * 100.f);
        window = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            win_w,
            win_h,
            SDL_WINDOW_SHOWN);
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    lcd_pixels = (uint8_t *)calloc((size_t)CUPCAKE_LCD_W * CUPCAKE_LCD_H, 4);
    if (!lcd_pixels) {
        fprintf(stderr, "LCD buffer alloc failed.\n");
        return 1;
    }

    bezel_tex = texture_from_rgba(renderer, bezel_px, bezel_w, bezel_h);
    atlas_tex = texture_from_rgba(renderer, atlas_pixels, atlas_w, atlas_h);
    lcd_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                CUPCAKE_LCD_W, CUPCAKE_LCD_H);
    if (lcd_tex) {
        SDL_SetTextureBlendMode(lcd_tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(lcd_tex, SDL_ScaleModeNearest);
    }

    stbi_image_free(bezel_px);

    if (!bezel_tex || !atlas_tex || !lcd_tex) {
        fprintf(stderr, "Texture upload failed.\n");
        return 1;
    }

    {
        char hiscore_path[512];
        char default_path[512];
        cupcake_hiscore_default_path(default_path, sizeof default_path);
        if (getenv("CUPCAKE_HISCORE_PATH"))
            snprintf(hiscore_path, sizeof hiscore_path, "%s", getenv("CUPCAKE_HISCORE_PATH"));
        else
            snprintf(hiscore_path, sizeof hiscore_path, "%s", default_path);
        cupcake_hiscore_set_path(hiscore_path);
        fprintf(stderr, "Hi-scores: %s\n", hiscore_path);
    }

    cupcake_init();
    cupcake_set_callback(cupcake_cb_wrap);

    if (host_audio_init(assets_base_dir()) != 0)
        fprintf(stderr, "Warning: SFX disabled — run: python tools/extract_audio.py\n");

    if (pin_tune_file_arg) {
        if (cupcake_set_debug_pin_from_tune(1) == 0)
            fprintf(stderr, "Warning: --pin but no sprites in assets/lcd_tune.txt\n");
    } else if (pin_sprite_arg) {
        if (pin_sprite_arg[0])
            cupcake_set_debug_pin(pin_sprite_arg, 1);
        else
            cupcake_set_debug_pin(NULL, 0);
    }
    if (pin_solo_arg)
        cupcake_set_debug_pin_solo(1);

    if (cupcake_debug_start_pending())
        cupcake_apply_debug_start();

    if (pin_tune_file_arg) {
        fprintf(stderr, "LCD alignment (edit assets/lcd_tune.txt — no rebuild):\n");
        cupcake_log_sprite_lcd(NULL);
    } else if (pin_sprite_arg && pin_sprite_arg[0]) {
        fprintf(stderr, "LCD alignment (edit assets/lcd_tune.txt — no rebuild):\n");
        cupcake_log_sprite_lcd(pin_sprite_arg);
    }

    fprintf(stderr,
            "\nCupcake Crisis PC test\n"
            "  Atlas: %s | Demo: demo.model attract (~4 fps)\n"
            "  Click the game window first, then keys below.\n"
            "  X / Select = cycle level (demo) | Z / Action = start / CON continue\n"
            "  1 / Level1, 2 / Level2 = quick start that level\n"
            "  Arrows / WASD = move (play, when enabled)\n"
            "  --pin = pin all sprites in assets/lcd_tune.txt (demo frozen)\n"
            "  --pin marge1 = pin one sprite; --pin off to clear\n"
            "  --pin-solo = only pinned sprite(s) on LCD (demo/attract tuning)\n"
            "  --align = --pin --pin-solo + green LCD border (edit assets/lcd_tune.txt)\n"
            "  Start play (Action) to preview catches with tune coords; pin sheet is demo-only.\n"
            "  --lcd-border = green outline of 1024x800 sprite buffer (auto with --pin)\n"
            "  --debug = lcd border + sprite stderr log\n"
            "  --start LEVEL,PHASE,SCORE = skip attract; jump to play (e.g. 1,1,9900)\n"
            "  CUPCAKE_DEBUG_START=1,1,9900 same as --start\n\n",
            atlas_file_loaded);

    while (run_loop) {
        input_poll();
        present();
        SDL_Delay(33);
    }

    host_audio_shutdown();
    free(atlas_pixels);
    free(lcd_pixels);
    SDL_DestroyTexture(lcd_tex);
    SDL_DestroyTexture(atlas_tex);
    SDL_DestroyTexture(bezel_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
