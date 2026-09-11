/* Compiled only when CUPCAKE_TRACE_SD=1 (see Makefile.gnw). */
#ifdef CUPCAKE_TRACE_SD

#include "cupcake_trace.h"

#include <stdarg.h>
#include <stdint.h>

#include "gw_firmware_abi.h"

#define CUPCAKE_TRACE_DIR    "/data/homebrew"
#define CUPCAKE_BUILD_ID     "cupcake-port-full gnw-embedded"

static char g_trace_path[96];
static int g_trace_ready;

static const gw_firmware_abi_t *trace_abi(void)
{
    return gw_firmware_abi();
}

const char *cupcake_trace_path(void)
{
    return g_trace_ready ? g_trace_path : "";
}

static void trace_vformat(char *buf, size_t bufsz, const char *fmt, va_list ap)
{
    const gw_firmware_abi_t *abi = trace_abi();

    if (!buf || bufsz == 0 || !fmt || !abi || !abi->vsnprintf)
        return;

    abi->vsnprintf(buf, bufsz, fmt, ap);
}

static void trace_format(char *buf, size_t bufsz, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    trace_vformat(buf, bufsz, fmt, ap);
    va_end(ap);
}

static void trace_build_path(void)
{
    const gw_firmware_abi_t *abi = trace_abi();
    unsigned year = 2000u;
    unsigned month = 1u;
    unsigned day = 1u;
    unsigned hour = 0u;
    unsigned min = 0u;
    unsigned sec = 0u;
    unsigned tick = 0u;

    if (!abi)
        return;

    if (abi->GW_GetCurrentYear)
        year = 2000u + (unsigned)abi->GW_GetCurrentYear();
    if (abi->GW_GetCurrentMonth)
        month = (unsigned)abi->GW_GetCurrentMonth();
    if (abi->GW_GetCurrentDay)
        day = (unsigned)abi->GW_GetCurrentDay();
    if (abi->GW_GetCurrentHour)
        hour = (unsigned)abi->GW_GetCurrentHour();
    if (abi->GW_GetCurrentMinute)
        min = (unsigned)abi->GW_GetCurrentMinute();
    if (abi->GW_GetCurrentSecond)
        sec = (unsigned)abi->GW_GetCurrentSecond();
    if (abi->HAL_GetTick)
        tick = (unsigned)(abi->HAL_GetTick() % 1000u);

    trace_format(g_trace_path, sizeof g_trace_path,
                 CUPCAKE_TRACE_DIR "/cupcake_%04u%02u%02u_%02u%02u%02u_%03u.log", year, month, day,
                 hour, min, sec, tick);
}

static void trace_write_line(const char *line, int create_new)
{
    const gw_firmware_abi_t *abi = trace_abi();
    FILE *fp;
    const char *mode;

    if (!g_trace_ready || !g_trace_path[0] || !line || !line[0])
        return;

    if (abi && abi->puts)
        abi->puts(line);

    if (!abi || !abi->fopen || !abi->fwrite || !abi->fclose)
        return;

    mode = create_new ? "w" : "a";
    fp = abi->fopen(g_trace_path, mode);
    if (!fp)
        return;

    if (abi->strlen && abi->fwrite)
        abi->fwrite(line, 1, abi->strlen(line), fp);
    if (abi->fputc)
        abi->fputc('\n', fp);
    abi->fclose(fp);
}

void cupcake_trace_init(void)
{
    const gw_firmware_abi_t *abi = trace_abi();

    g_trace_ready = 0;
    g_trace_path[0] = '\0';

    if (abi && abi->odroid_sdcard_mkdir)
        abi->odroid_sdcard_mkdir(CUPCAKE_TRACE_DIR);

    trace_build_path();
    if (!g_trace_path[0])
        return;

    g_trace_ready = 1;
    trace_write_line("=== session " CUPCAKE_BUILD_ID " ===", 1);
    cupcake_trace("log: %s", g_trace_path);
}

void cupcake_trace(const char *fmt, ...)
{
    char line[320];
    va_list ap;

    if (!g_trace_ready || !fmt)
        return;

    va_start(ap, fmt);
    trace_vformat(line, sizeof line, fmt, ap);
    va_end(ap);
    trace_write_line(line, 0);
}

#endif /* CUPCAKE_TRACE_SD */
