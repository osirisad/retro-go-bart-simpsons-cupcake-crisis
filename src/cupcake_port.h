#ifndef CUPCAKE_PORT_H_
#define CUPCAKE_PORT_H_

/* Sprite LCD = full visible bezel in screen.jpg (top 800px; case strip below is unused). */
#define CUPCAKE_LCD_ATLAS_X        0
#define CUPCAKE_LCD_ATLAS_Y        0
#define CUPCAKE_LCD_ATLAS_W        1024
#define CUPCAKE_BEZEL_VISIBLE_H    800
#define CUPCAKE_LCD_ATLAS_H        CUPCAKE_BEZEL_VISIBLE_H

/* LCD logical size (matches atlas playfield in sprites-color.png). */
#define CUPCAKE_LCD_W         CUPCAKE_LCD_ATLAS_W
#define CUPCAKE_LCD_H         CUPCAKE_LCD_ATLAS_H

/*
 * Alignment tuning: uncomment to always draw one sprite (demo frozen).
 * PC: ./cupcake-sdl.exe --pin --pin-solo  (pins all lines in assets/lcd_tune.txt)
 */
/* #define CUPCAKE_DEBUG_PIN_SPRITE_STR "marge1" */

#endif
