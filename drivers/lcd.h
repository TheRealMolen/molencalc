#pragma once

#include "pico/stdlib.h"
#include "font.h"


// LCD display parameters
#define WIDTH           (320)           // pixels across the LCD
#define HEIGHT          (320)           // pixels down the LCD

// Handy macros
#define RGB(r,g,b)      ((uint16_t)(((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3)))
#define UPPER8(x)       ((x) >> 8)      // upper byte of a 16-bit value
#define LOWER8(x)       ((x) & 0xFF)    // lower byte of a 16-bit value


// Initialization
void lcd_clear_screen();
void lcd_init();

// Low-level drawing
void lcd_blit(const uint16_t *pixels, int x, int y, int width, int height);
void lcd_solid_rectangle(uint16_t colour, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

// Scrolling
void lcd_define_scrolling(uint16_t top_fixed_area, uint16_t bottom_fixed_area);
void lcd_scroll_reset();
void lcd_scroll_clear(uint16_t col = 0);
void lcd_scroll_up(uint32_t distance);
void lcd_scroll_down(uint32_t distance);
