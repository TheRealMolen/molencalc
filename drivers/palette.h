#pragma once

#include <stdint.h>


//----------------------------------------------------------------------------------------

#define RGB16(r,g,b)    \
    ((uint16_t)((((r) & 0xf8) << 8) | (((g) &0xfc) << 3) | ((((uint8_t)(b)) >> 3))))

#define RGB16_r(rgb)    (((rgb) >> 8) & 0xf8)
#define RGB16_g(rgb)    (((rgb) >> 3) & 0xfc)
#define RGB16_b(rgb)    (((rgb) << 3) & 0xf8)

//----------------------------------------------------------------------------------------

typedef struct Palette
{
    uint8_t Len;
    const uint16_t *Cols;

    inline void Inflate(uint16_t* outPix16, const uint8_t* inPix8, int num) const;

    inline void ExportAsRGB(uint8_t* outRgb, int numEntries) const;
    inline void ExportAsBGRQuads(uint8_t* outRgbx, int numEntries) const;

} Palette;

//----------------------------------------------------------------------------------------

inline void Palette::Inflate(uint16_t* outPix16, const uint8_t* inPix8, int num) const
{
    const uint8_t* inEnd = inPix8 + num;
    
    for (; inPix8 != inEnd; ++inPix8, ++outPix16)
    {
        *outPix16 = Cols[*inPix8];
    }
}

//----------------------------------------------------------------------------------------

inline void Palette::ExportAsRGB(uint8_t* outRgb, int numEntries) const
{
    int ix = 0;
    for (; ix < Len && ix < numEntries; ++ix, outRgb += 3)
    {
        const uint16_t col = Cols[ix];

        outRgb[0] = RGB16_r(col);
        outRgb[1] = RGB16_g(col);
        outRgb[2] = RGB16_b(col);
    }
    for (; ix < numEntries; ++ix, outRgb += 3)
    {
        outRgb[0] = 0;
        outRgb[1] = 0;
        outRgb[2] = 0;
    }
}

//----------------------------------------------------------------------------------------

inline void Palette::ExportAsBGRQuads(uint8_t* outRgbx, int numEntries) const
{
    int ix = 0;
    for (; ix < Len && ix < numEntries; ++ix, outRgbx += 4)
    {
        const uint16_t col = Cols[ix];

        outRgbx[0] = RGB16_b(col);
        outRgbx[1] = RGB16_g(col);
        outRgbx[2] = RGB16_r(col);
        outRgbx[3] = 0;
    }
    for (; ix < numEntries; ++ix, outRgbx += 4)
    {
        outRgbx[0] = 0;
        outRgbx[1] = 0;
        outRgbx[2] = 0;
        outRgbx[3] = 0;
    }
}

//----------------------------------------------------------------------------------------
