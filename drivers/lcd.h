#pragma once

#include <cstdint>
#include "platform.h"

//----------------------------------------------------------------------------------------

// LCD display parameters
#define WIDTH           (320)           // pixels across the LCD
#define HEIGHT          (320)           // pixels down the LCD

// Handy macros
#define RGB(r,g,b)      ((uint16_t)(((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3)))
#define UPPER8(x)       ((x) >> 8)      // upper byte of a 16-bit value
#define LOWER8(x)       ((x) & 0xFF)    // lower byte of a 16-bit value

//----------------------------------------------------------------------------------------

// Initialization
bool lcd_init();
void lcd_cleanup();

#ifdef MLN_TARGET_PC
typedef struct SDL_Window SDL_Window;
void lcd_refresh(SDL_Window* window);
#endif

void lcd_clear_screen(uint16_t col = 0);

// Low-level drawing
void lcd_blit(const uint16_t *pixels, int x, int y, int width, int height);
void lcd_rect(int x, int y, int w, int h, uint16_t col);

void lcd_readback(int x, int y, int width, int height, uint16_t *out_pixels);

// Scrolling
void lcd_define_scrolling(uint16_t top_fixed_area, uint16_t bottom_fixed_area);
void lcd_scroll_reset();
void lcd_scroll_clear(uint16_t col = 0);
void lcd_scroll_up(uint32_t distance, uint16_t clearCol = 0);
void lcd_scroll_down(uint32_t distance, uint16_t clearCol = 0);

//----------------------------------------------------------------------------------------
