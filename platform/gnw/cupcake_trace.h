#pragma once

/* Per-session debug log on SD: /retro-go/saves/cupcake_YYYYMMDD_HHMMSS.log
 * Pull the SD card and open the newest file on a PC — no debug probe required. */

void cupcake_trace_init(void);
void cupcake_trace(const char *fmt, ...);

/* Path chosen at init (empty until cupcake_trace_init runs). */
const char *cupcake_trace_path(void);
