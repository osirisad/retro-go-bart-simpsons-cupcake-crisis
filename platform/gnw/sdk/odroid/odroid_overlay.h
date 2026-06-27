#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "odroid_input.h"

typedef enum {
    ODROID_DIALOG_INIT,
    ODROID_DIALOG_PREV,
    ODROID_DIALOG_NEXT,
    ODROID_DIALOG_FOCUS_GAINED,
    ODROID_DIALOG_ENTER,
} odroid_dialog_event_t;

typedef struct odroid_dialog_choice odroid_dialog_choice_t;

struct odroid_dialog_choice {
    int id;
    const char *label;
    char *value;
    int enabled;
    bool (*update_cb)(odroid_dialog_choice_t *, odroid_dialog_event_t, uint32_t repeat);
};

#define ODROID_DIALOG_CHOICE_LAST {0x0F0F0F0F, "LAST", (char *)"LAST", 0xFFFF, NULL}

void odroid_overlay_alert(const char *text);
