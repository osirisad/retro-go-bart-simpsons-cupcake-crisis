#pragma once

/* Per-session SD debug log: /data/homebrew/cupcake_YYYYMMDD_HHMMSS.log
 * Off by default. Enable at build time: make cupcake-bin CUPCAKE_TRACE_SD=1 */

#ifdef CUPCAKE_TRACE_SD

void cupcake_trace_init(void);
void cupcake_trace(const char *fmt, ...);

/* Path chosen at init (empty until cupcake_trace_init runs). */
const char *cupcake_trace_path(void);

#else

#define cupcake_trace_init() ((void)0)
#define cupcake_trace(...)   ((void)0)

static inline const char *cupcake_trace_path(void)
{
    return "";
}

#endif
