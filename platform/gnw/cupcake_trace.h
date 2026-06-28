#pragma once

/* Append-only debug log on SD: /retro-go/saves/cupcake_debug.log
 * Pull the SD card and open that file on a PC — no debug probe required. */

void cupcake_trace_init(void);
void cupcake_trace(const char *fmt, ...);
