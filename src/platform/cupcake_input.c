#include "cupcake_input.h"

#if !defined(CUPCAKE_GNW) && !defined(TARGET_GNW)

#include <SDL2/SDL_scancode.h>

void cupcake_input_from_sdl_keyboard(const uint8_t *keys, uint16_t *buttons)
{
    uint16_t b = 0;

    if (!keys || !buttons)
        return;

    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
        b |= CUPCAKE_BTN_LEFT;
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
        b |= CUPCAKE_BTN_RIGHT;
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W])
        b |= CUPCAKE_BTN_UP;
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S])
        b |= CUPCAKE_BTN_DOWN;
    if (keys[SDL_SCANCODE_Z])
        b |= CUPCAKE_BTN_ACTION;
    if (keys[SDL_SCANCODE_X])
        b |= CUPCAKE_BTN_SELECT;
    if (keys[SDL_SCANCODE_1])
        b |= CUPCAKE_BTN_LEVEL1;
    if (keys[SDL_SCANCODE_2])
        b |= CUPCAKE_BTN_LEVEL2;
    if (keys[SDL_SCANCODE_F6])
        b |= CUPCAKE_BTN_SOUND;

    *buttons = b;
}

#endif /* !CUPCAKE_GNW */

#if defined(TARGET_GNW) || defined(LINUX_EMU) || defined(HOST_BUILD)
#include "odroid_input.h"

void cupcake_input_from_odroid(const odroid_gamepad_state_t *pad, uint16_t *buttons)
{
    uint16_t b = 0;

    if (!pad || !buttons)
        return;

    if (pad->values[ODROID_INPUT_LEFT])
        b |= CUPCAKE_BTN_LEFT;
    if (pad->values[ODROID_INPUT_RIGHT])
        b |= CUPCAKE_BTN_RIGHT;
    if (pad->values[ODROID_INPUT_UP])
        b |= CUPCAKE_BTN_UP;
    if (pad->values[ODROID_INPUT_DOWN])
        b |= CUPCAKE_BTN_DOWN;
    /* G&W A + GAME = Action / Start (itch.io btnAction). */
    if (pad->values[ODROID_INPUT_A] || pad->values[ODROID_INPUT_START])
        b |= CUPCAKE_BTN_ACTION;
    /* G&W B + TIME = Select — cycle demo level 0→1→2→0 (btnSelect). */
    if (pad->values[ODROID_INPUT_B] || pad->values[ODROID_INPUT_SELECT])
        b |= CUPCAKE_BTN_SELECT;
#if !defined(CUPCAKE_GNW) && !defined(TARGET_GNW) && !defined(HOST_BUILD)
    /* G&W PAUSE/SET is retro-go menu + volume — do not map to in-game mute on device. */
    if (pad->values[ODROID_INPUT_VOLUME])
        b |= CUPCAKE_BTN_SOUND;
#endif

    *buttons = b;
}
#endif
