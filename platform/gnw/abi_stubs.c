/*
 * Firmware API shims — call through gw_firmware_abi (VTOR+0x400), not link-time symbols.
 */
#include "rg_abi.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reent.h>
#include <time.h>

static int g_abi_sample_rate = 22050;

#define A (gw_firmware_abi())

struct _reent *_impure_ptr;

__attribute__((used, section(".gw_core_bridge_probe")))
const uint32_t GW_CORE_BUILT_ABI_VERSION = GW_FIRMWARE_ABI_VERSION;

__attribute__((used, section(".gw_core_bridge_probe")))
const uint32_t GW_CORE_BUILT_ABI_SIZE = sizeof(gw_firmware_abi_t);

void gw_abi_bind_stdio(void)
{
    if (A->impure_ptr_ptr)
        _impure_ptr = *(struct _reent **)(A->impure_ptr_ptr);
}

/* --- libc / string -------------------------------------------------------- */

void *memcpy(void *d, const void *s, size_t n)
{
    return A->memcpy(d, s, n);
}

void *memset(void *d, int c, size_t n)
{
    return A->memset(d, c, n);
}

void *memmove(void *d, const void *s, size_t n)
{
    return A->memmove(d, s, n);
}

void *malloc(size_t size)
{
    return A->ram_malloc(size);
}

void free(void *p)
{
    A->free(p);
}

void *realloc(void *p, size_t size)
{
    return A->realloc(p, size);
}

void *calloc(size_t count, size_t size)
{
    void *p = A->ram_malloc(count * size);

    if (p)
        A->memset(p, 0, count * size);
    return p;
}

int strcmp(const char *a, const char *b)
{
    return A->strcmp(a, b);
}

int strncmp(const char *a, const char *b, size_t n)
{
    return A->strncmp(a, b, n);
}

size_t strlen(const char *s)
{
    return A->strlen(s);
}

char *strcpy(char *dst, const char *src)
{
    size_t n;

    if (!dst || !src)
        return dst;
    n = A->strlen(src) + 1;
    A->memcpy(dst, src, n);
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    return A->memcmp(a, b, n);
}

long strtol(const char *s, char **end, int base)
{
    return A->strtol(s, end, base);
}

unsigned long strtoul(const char *s, char **end, int base)
{
    return (unsigned long)A->strtol(s, end, base);
}

char *getenv(const char *name)
{
    (void)name;
    return NULL;
}

char *fgets(char *buf, int size, FILE *stream)
{
    return A->fgets(buf, size, stream);
}

int fgetc(FILE *stream)
{
    return A->fgetc(stream);
}

int ungetc(int c, FILE *stream)
{
    return A->ungetc(c, stream);
}

int fputs(const char *s, FILE *stream)
{
    if (!s)
        return EOF;
    while (*s) {
        if (A->fputc((unsigned char)*s++, stream) == EOF)
            return EOF;
    }
    return 0;
}

time_t time(time_t *t)
{
    return A->time(t);
}

double pow(double x, double y)
{
    return A->pow(x, y);
}

void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    A->__assert_func(file, line, func, expr);
}

int printf(const char *fmt, ...)
{
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = A->vprintf(fmt, ap);
    va_end(ap);
    return r;
}

int fprintf(FILE *stream, const char *fmt, ...)
{
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = A->vfprintf(stream, fmt, ap);
    va_end(ap);
    return r;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = A->vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return r;
}

FILE *fopen(const char *path, const char *mode)
{
    return A->fopen(path, mode);
}

int fclose(FILE *stream)
{
    return A->fclose(stream);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return A->fread(ptr, size, nmemb, stream);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return A->fwrite(ptr, size, nmemb, stream);
}

int fseek(FILE *stream, long offset, int whence)
{
    return A->fseek(stream, offset, whence);
}

long ftell(FILE *stream)
{
    return A->ftell(stream);
}

/* --- LCD / audio hardware ------------------------------------------------- */

void lcd_swap(void)
{
    A->wdog_refresh();
    A->lcd_swap();
}

void *lcd_get_active_buffer(void)
{
    return A->lcd_get_active_buffer();
}

void lcd_clear_buffers(void)
{
    A->lcd_clear_active_buffer();
    A->lcd_clear_inactive_buffer();
}

void audio_start_playing(uint16_t length)
{
    A->audio_start_playing(length);
}

void odroid_audio_submit(short *stereoAudioBuffer, int frameCount)
{
    int16_t *dst;
    int i;
    uint8_t vol;

    if (!stereoAudioBuffer || frameCount <= 0)
        return;

    if (A->common_emu_sound_loop_is_muted && A->common_emu_sound_loop_is_muted())
        return;

    dst = A->audio_get_active_buffer();
    if (!dst)
        return;

    vol = A->common_emu_sound_get_volume ? A->common_emu_sound_get_volume() : (uint8_t)255;

    /* >>4 matches GW emu ports (firmware volume stacks on top). */
    for (i = 0; i < frameCount; i++) {
        int32_t sample = (int32_t)stereoAudioBuffer[i] * (int32_t)vol;

        sample >>= 4;
        if (sample > 32767)
            sample = 32767;
        if (sample < -32768)
            sample = -32768;
        dst[i] = (int16_t)sample;
    }
}

int odroid_audio_sample_rate_get(void)
{
    return g_abi_sample_rate;
}

/* --- retro-go system / input / overlay ------------------------------------ */

void odroid_system_init(int app_id, int sample_rate)
{
    (void)app_id;
    g_abi_sample_rate = sample_rate;
    A->odroid_system_init(app_id, sample_rate);
}

void odroid_system_emu_init(state_handler_t load_cb, state_handler_t save_cb,
                            screenshot_handler_t screenshot_cb, shutdown_handler_t shutdown_cb,
                            sleep_post_wakeup_handler_t sleep_post_wakeup_cb,
                            sram_save_handler_t sram_save_cb)
{
    A->odroid_system_emu_init(load_cb, save_cb, screenshot_cb, shutdown_cb, sleep_post_wakeup_cb,
                              sram_save_cb);
}

bool odroid_system_emu_load_state(int slot)
{
    return A->odroid_system_emu_load_state(slot);
}

void odroid_input_read_gamepad(odroid_gamepad_state_t *out_state)
{
    A->wdog_refresh();
    A->odroid_input_read_gamepad(out_state);
}

void odroid_overlay_alert(const char *text)
{
    if (!text)
        return;
    A->odroid_overlay_draw_text(8, 40, 304, text, 0xFFFF, 0x0000);
    if (A->HAL_Delay)
        A->HAL_Delay(2500);
}

/* --- common emulator loop ------------------------------------------------- */

bool common_emu_frame_loop(void)
{
    return A->common_emu_frame_loop();
}

void common_emu_input_loop(odroid_gamepad_state_t *joystick, odroid_dialog_choice_t *game_options,
                           void (*repaint)(void))
{
    A->common_emu_input_loop(joystick, game_options, repaint);
}

void common_emu_input_loop_handle_turbo(odroid_gamepad_state_t *joystick)
{
    A->common_emu_input_loop_handle_turbo(joystick);
}

void common_emu_sound_sync(bool use_nops)
{
    A->common_emu_sound_sync(use_nops);
}

void wdog_refresh(void)
{
    A->wdog_refresh();
}

void *ram_malloc(size_t size)
{
    return A->ram_malloc(size);
}
