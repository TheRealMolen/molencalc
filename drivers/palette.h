#pragma once

#include <stdint.h>


//----------------------------------------------------------------------------------------

#define RGB16(r,g,b)    \
    ((uint16_t)((((r) & 0xf8) << 8) | (((g) &0xfc) << 3) | ((((uint8_t)(b)) >> 3))))

//----------------------------------------------------------------------------------------

typedef struct Palette
{
    uint8_t Len;
    const uint16_t *Cols;

    inline void Inflate(uint16_t* outPix16, const uint8_t* inPix8, int num) const;

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
