#pragma once

#include <stdint.h>
#include "platform.h"

//----------------------------------------------------------------------------------------

#define LCD_USEPALETTE  1

//----------------------------------------------------------------------------------------

typedef uint8_t col8_t;
typedef uint16_t col16_t;

#if LCD_USEPALETTE
typedef col8_t col_t;
#else
typedef col16_t col_t;
#endif

typedef struct Palette Palette;

//----------------------------------------------------------------------------------------

#define PAL_BG          (0)
#define PAL_FG          (1)
#define PAL_PLOTAREA    (2)
#define PAL_PLOTAXIS    (3)
#define PAL_PLOTCOLS    (4)
#define PAL_FBGRADIENT  (16)

#define COL_DEFAULT_BG  (0x0000)
#define COL_DEFAULT_FG  (0xFF07)

//----------------------------------------------------------------------------------------
