#include "font.h"

#include <algorithm>


//----------------------------------------------------------------------------------------

GlyphMetric font_get_glyph_metric(const Font* font, char c, bool monospace)
{
    const uint8_t glyphWidth = font->Width;

    if (monospace)
        return { .Skip = 0, .Advance = glyphWidth };

    if (font == &font_10x16)
    {
        if (c == ' ')
            return { .Skip = 0, .Advance = uint8_t(glyphWidth - 2) };
        if (c == '\'' || c == '.' || c == ',')
            return { .Skip = 3, .Advance = uint8_t(glyphWidth - 7) };
        if (c == ';' || c == ':')
            return { .Skip = 2, .Advance = uint8_t(glyphWidth - 5) };
        if (c == 'i')
            return { .Skip = 1, .Advance = uint8_t(glyphWidth - 4) };
        if (c == 'l' || c == '1')
            return { .Skip = 2, .Advance = uint8_t(glyphWidth - 4) };
        if (c == 'f' || c == 't' || c == '^')
            return { .Skip = 1, .Advance = uint8_t(glyphWidth - 2) };
        if (c == 'r')
            return { .Skip = 0, .Advance = uint8_t(glyphWidth - 1) };
        if (c == 'k')
            return { .Skip = 0, .Advance = uint8_t(glyphWidth - 2) };
    }
    else if (font == &font_5x10)
    {
        if (c == ' ')
            return { .Skip = 0, .Advance = uint8_t(glyphWidth - 1) };
        if (c == 'm' || c == '/' || c == 'w' || c == 'v' || c == 'W' || c == 'V' || c == 'x')
            return { .Skip = 0, .Advance = uint8_t(glyphWidth + 1) };
        if (c == 'l' || c == 'I' || c == '1' || c == 'i')
            return { .Skip = 1, .Advance = uint8_t(glyphWidth - 1) };
    }

    return { .Skip = 0, .Advance = glyphWidth };
}

//----------------------------------------------------------------------------------------

void font_rasterise_char(
    const Font* font,
    char c,
    uint16_t fgcol,
    uint16_t bgcol,
    uint16_t* buf,
    int bufw,
    int bufh,
    int x,
    int y)
{
    const int glyphWidth = font->Width;
    const int fullGlyphHeight = font->Height;
    const int bytesPerGlyphRow = (glyphWidth + 7) / 8;

    const uint8_t* glyph = &font->Glyphs[c * fullGlyphHeight * bytesPerGlyphRow];

    uint16_t* outPix = buf + x + (y*bufw);
    const int pitch = bufw;
    
    const int glyphHeight = std::min(fullGlyphHeight, bufh-y);

    for (int i = 0; i < glyphHeight; ++i, glyph+=bytesPerGlyphRow, outPix+=pitch)
    {
        uint16_t g = *glyph;

        if (glyphWidth == 5)
        {
            outPix[0] = (g & 0x10) ? fgcol : bgcol;
            outPix[1] = (g & 0x08) ? fgcol : bgcol;
            outPix[2] = (g & 0x04) ? fgcol : bgcol;
            outPix[3] = (g & 0x02) ? fgcol : bgcol;
            outPix[4] = (g & 0x01) ? fgcol : bgcol;
        }
        else if (glyphWidth == 10)
        {
            g = (g << 8) | glyph[1];

            outPix[0] = (g & 0x200) ? fgcol : bgcol;
            outPix[1] = (g & 0x100) ? fgcol : bgcol;
            outPix[2] = (g & 0x080) ? fgcol : bgcol;
            outPix[3] = (g & 0x040) ? fgcol : bgcol;
            outPix[4] = (g & 0x020) ? fgcol : bgcol;
            outPix[5] = (g & 0x010) ? fgcol : bgcol;
            outPix[6] = (g & 0x008) ? fgcol : bgcol;
            outPix[7] = (g & 0x004) ? fgcol : bgcol;
            outPix[8] = (g & 0x002) ? fgcol : bgcol;
            outPix[9] = (g & 0x001) ? fgcol : bgcol;
        }
    }
}

//----------------------------------------------------------------------------------------

