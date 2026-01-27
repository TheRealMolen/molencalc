#pragma once

#include <pico/stdlib.h>

#ifndef LARGE_FONT
#define LARGE_FONT 1
#endif


typedef struct {
    uint8_t width;
    uint8_t glyphs[];
} font_t;


#if LARGE_FONT

#define GLYPH_HEIGHT 16         // Height of each glyph in pixels
extern const font_t font_10x16;

#else

#define GLYPH_HEIGHT 10         // Height of each glyph in pixels
extern const font_t font_5x10;  // 5x10 pixel font

#endif