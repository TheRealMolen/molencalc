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

#define GFX_USEFRAMEBUF 1

//----------------------------------------------------------------------------------------
// palette & frame buffer

typedef struct Palette Palette;

void gfx_set_palette(const Palette *new_palette);
const Palette* gfx_get_palette();

void fb_blitline(int x, int y, int width, const col8_t* pixels);
void fb_readback(int x, int y, int width, int height, col8_t *out_pixels);

//----------------------------------------------------------------------------------------

