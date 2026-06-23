#include "cupcake_input.h"

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

    *buttons = b;
}

#ifdef TARGET_GNW
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
    /* G&W A = Action (matches celeste bit 4 on device). */
    if (pad->values[ODROID_INPUT_A])
        b |= CUPCAKE_BTN_ACTION;
    /* G&W B = Select / mode. */
    if (pad->values[ODROID_INPUT_B])
        b |= CUPCAKE_BTN_SELECT;
    if (pad->values[ODROID_INPUT_START])
        b |= CUPCAKE_BTN_LEVEL1;
    if (pad->values[ODROID_INPUT_X])
        b |= CUPCAKE_BTN_LEVEL2;

    *buttons = b;
}
#endif
