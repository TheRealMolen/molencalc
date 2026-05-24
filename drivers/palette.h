#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------------------

#define RGB16(r,g,b)    \
    ((uint16_t)((((r) & 0xf8) << 8) | (((g) &0xfc) << 3) | ((((uint8_t)(b)) >> 3))))

//----------------------------------------------------------------------------------------

typedef struct Palette
{
    uint8_t Len;
    const uint16_t *Cols;
} Palette;


#ifdef __cplusplus
};
#endif

