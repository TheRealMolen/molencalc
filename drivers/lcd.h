#pragma once

#include <stdint.h>

#include "platform.h"
#include "colours.h"

//----------------------------------------------------------------------------------------

// LCD display parameters
#define WIDTH           (320)           // pixels across the LCD
#define HEIGHT          (320)           // pixels down the LCD
#define BACKBUF_HEIGHT  (480)           // frame memory height in pixels

// Handy macros
#define UPPER8(x)       ((x) >> 8)      // upper byte of a 16-bit value
#define LOWER8(x)       ((x) & 0xFF)    // lower byte of a 16-bit value

#define LCD_USEFRAMEBUF 0

//----------------------------------------------------------------------------------------

// Initialization
bool lcd_init(col_t clearCol);
void lcd_cleanup();

#ifdef MLN_TARGET_PC
typedef struct SDL_Window SDL_Window;
void lcd_refresh(SDL_Window* window);
#endif

//----------------------------------------------------------------------------------------
// palette & frame buffer

typedef struct Palette Palette;

void gfx_set_palette(const Palette *new_palette);
const Palette* gfx_get_palette();

void fb_blitline(int x, int y, int width, const col8_t* pixels);
void fb_readback(int x, int y, int width, int height, col8_t *out_pixels);

//----------------------------------------------------------------------------------------

void lcd_clear_screen(col_t col = 0);

// Low-level drawing
void lcd_blit(const col_t *pixels, int x, int y, int width, int height);
void lcd_rect(int x, int y, int w, int h, col_t col);

void lcd_readback(int x, int y, int width, int height, col16_t *out_pixels);

// Scrolling
void lcd_define_scrolling(uint16_t top_fixed_area, uint16_t bottom_fixed_area);
void lcd_scroll_reset();
void lcd_scroll_clear(col_t col = 0);
void lcd_scroll_up(uint32_t distance, col_t clearCol = 0);
void lcd_scroll_down(uint32_t distance, col_t clearCol = 0);

//----------------------------------------------------------------------------------------
