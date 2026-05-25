#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "colours.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------------------

#define FONT_MAX_WIDTH  12
#define FONT_MAX_HEIGHT 16

//----------------------------------------------------------------------------------------

typedef struct Font
{
    uint8_t Width;
    uint8_t Height;
    uint8_t Glyphs[];
} Font;

typedef struct GlyphMetric
{
    uint8_t Skip;
    uint8_t Advance;
} GlyphMetric;


//----------------------------------------------------------------------------------------

GlyphMetric font_get_glyph_metric(const Font* font, char c, bool monospace);

void font_rasterise_char(
    const Font* font,
    char c,
    col_t fgcol,
    col_t bgcol,
    col_t* buf,
    int bufw,
    int bufh,
    int x,
    int y);

extern const Font font_5x10;
extern const Font font_10x16;

//----------------------------------------------------------------------------------------

#ifdef __cplusplus
};
#endif

