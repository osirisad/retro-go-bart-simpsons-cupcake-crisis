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
 * Alignment tuning:
 *   run-align.bat              static pin sheet (edit assets/lcd_tune.txt)
 *   ./cupcake-sdl.exe --align  same (--pin --pin-solo + LCD border)
 * Live override: lcd_tune.txt reloaded each frame (same coords as header after bake).
 * Bake into cupcake_sprite_lcd.h:  make bake-lcd   (or make gen when PIL available)
 * Regenerate tune layout only:     python tools/gen_lcd_tune_align.py
 */
/* #define CUPCAKE_DEBUG_PIN_SPRITE_STR "marge1" */

#endif
