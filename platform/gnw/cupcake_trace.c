#include "cupcake_trace.h"

#include <stdarg.h>
#include <stdint.h>

#include "gw_firmware_abi.h"

#define CUPCAKE_TRACE_PATH "/retro-go/saves/cupcake_debug.log"
#define CUPCAKE_BUILD_ID   "cupcake-port-full gnw-embedded"

static const gw_firmware_abi_t *trace_abi(void)
{
    return gw_firmware_abi();
}

static void trace_write_line(const char *line)
{
    const gw_firmware_abi_t *abi = trace_abi();
    FILE *fp;

    if (!abi || !line || !line[0])
        return;

    if (abi->puts)
        abi->puts(line);

    if (!abi->fopen || !abi->fwrite || !abi->fclose)
        return;

    fp = abi->fopen(CUPCAKE_TRACE_PATH, "a");
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

    if (abi && abi->odroid_sdcard_mkdir)
        abi->odroid_sdcard_mkdir("/retro-go/saves");

    cupcake_trace("=== session %s ===", CUPCAKE_BUILD_ID);
}

void cupcake_trace(const char *fmt, ...)
{
    char line[320];
    va_list ap;
    const gw_firmware_abi_t *abi = trace_abi();

    if (!abi || !fmt || !abi->vsnprintf)
        return;

    va_start(ap, fmt);
    abi->vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    trace_write_line(line);
}
