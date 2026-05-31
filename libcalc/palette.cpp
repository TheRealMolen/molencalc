#include "palette.h"

#include "drivers/gfx.h"
#include "drivers/palette.h"

//-------------------------------------------------------------------------------------------------

namespace
{
    // warmly darken an RGB565 colour by ~10%
    constexpr uint16_t darken(uint16_t c)
    {
        uint16_t r = c >> 11;
        uint16_t g = (c >> 5) & 0x3f;
        uint16_t b = c & 0x1f;

        r = (r * 15) / 16;
        g = (g * 29) / 32;
        b = (b * 14) / 16;

        return (r << 11) | (g << 5) | (b);
    }

    template<unsigned int count>
    constexpr uint16_t darkenTimes(uint16_t c)
    {
        return darkenTimes<count - 1>(darken(c));
    }
    template<>
    constexpr uint16_t darkenTimes<0>(uint16_t c)
    {
        return c;
    }

    // lighten an RGB565 colour by ~10%
    constexpr uint16_t lighten(uint16_t c)
    {
        uint16_t r = c >> 11;
        uint16_t g = (c >> 5) & 0x3f;
        uint16_t b = c & 0x1f;

        r = (r * 20) / 16;
        g = (g * 35) / 32;
        b = (b * 20) / 16;

        if (r > 0x1f) r = 0x1f;
        if (g > 0x3f) g = 0x3f;
        if (b > 0x1f) b = 0x1f;

        return (r << 11) | (g << 5) | (b);
    }

    template<unsigned int count>
    constexpr uint16_t lightenTimes(uint16_t c)
    {
        return lightenTimes<count - 1>(lighten(c));
    }
    template<>
    constexpr uint16_t lightenTimes<0>(uint16_t c)
    {
        return c;
    }

    static constexpr uint16_t COL_AMBER = RGB16(255,227, 57);
    static constexpr uint16_t COL_AQUA = 0xb7f5;
    static constexpr uint16_t COL_TinyScope_DarkMode = COL_AMBER;

    static constexpr uint16_t COL_TinyScope_LiteMode = RGB16( 35, 91,116);
}

static constexpr uint16_t gDarkModeCols[] =
{
    RGB16(  0,  0,  0),     // default background
    RGB16(255,227, 57),     // default foreground

    RGB16( 49, 49, 49),     // graph area
    RGB16(112,112, 16),     // graph axes

    RGB16(224, 96,144),     // graph line 2
    RGB16( 29,179, 71),     // graph line 3
    RGB16( 63,153,192),     // graph line 4
    RGB16(194, 95,246),     // graph line 5

    0, 0, 0, 0,             // reserved
    0, 0, 0, 0,

    // tinyscope colours - gradient black -> col
    0,
    darkenTimes<14>(COL_TinyScope_DarkMode),
    darkenTimes<13>(COL_TinyScope_DarkMode),
    darkenTimes<12>(COL_TinyScope_DarkMode),
    darkenTimes<11>(COL_TinyScope_DarkMode),
    darkenTimes<10>(COL_TinyScope_DarkMode),
    darkenTimes< 9>(COL_TinyScope_DarkMode),
    darkenTimes< 8>(COL_TinyScope_DarkMode),
    darkenTimes< 7>(COL_TinyScope_DarkMode),
    darkenTimes< 6>(COL_TinyScope_DarkMode),
    darkenTimes< 5>(COL_TinyScope_DarkMode),
    darkenTimes< 4>(COL_TinyScope_DarkMode),
    darkenTimes< 3>(COL_TinyScope_DarkMode),
    darkenTimes< 2>(COL_TinyScope_DarkMode),
    darkenTimes< 1>(COL_TinyScope_DarkMode),
    darkenTimes< 0>(COL_TinyScope_DarkMode),
};
static constexpr uint8_t kNumDarkModeCols = sizeof(gDarkModeCols) / sizeof(gDarkModeCols[0]);
static_assert(kNumDarkModeCols == 32);

static const Palette gPaletteDark { kNumDarkModeCols, gDarkModeCols };


static constexpr uint16_t gLiteModeCols[] =
{
    RGB16(200,216,208),     // default background
    RGB16( 26, 96,104),     // default foreground

    RGB16(176,192,184),     // graph area
    RGB16(128,144,136),     // graph axes

    RGB16(209, 92,127),     // graph line 2
    RGB16( 60,157, 87),     // graph line 3
    RGB16(210,133, 17),     // graph line 4
    RGB16(  0,130,161),     // graph line 5

    0, 0, 0, 0,             // reserved
    0, 0, 0, 0,

    // tinyscope colours
    0xffff,
    lightenTimes<14>(COL_TinyScope_LiteMode),
    lightenTimes<13>(COL_TinyScope_LiteMode),
    lightenTimes<12>(COL_TinyScope_LiteMode),
    lightenTimes<11>(COL_TinyScope_LiteMode),
    lightenTimes<10>(COL_TinyScope_LiteMode),
    lightenTimes< 9>(COL_TinyScope_LiteMode),
    lightenTimes< 8>(COL_TinyScope_LiteMode),
    lightenTimes< 7>(COL_TinyScope_LiteMode),
    lightenTimes< 6>(COL_TinyScope_LiteMode),
    lightenTimes< 5>(COL_TinyScope_LiteMode),
    lightenTimes< 4>(COL_TinyScope_LiteMode),
    lightenTimes< 3>(COL_TinyScope_LiteMode),
    lightenTimes< 2>(COL_TinyScope_LiteMode),
    lightenTimes< 1>(COL_TinyScope_LiteMode),
    lightenTimes< 0>(COL_TinyScope_LiteMode),
};
static constexpr uint8_t kNumLiteModeCols = sizeof(gLiteModeCols) / sizeof(gLiteModeCols[0]);
static_assert(kNumLiteModeCols == 32);

static const Palette gPaletteLite { kNumLiteModeCols, gLiteModeCols };

//-------------------------------------------------------------------------------------------------

const Palette* palette_get_lite()
{
    return &gPaletteLite;
}

const Palette* palette_get_dark()
{
    return &gPaletteDark;
}

//-------------------------------------------------------------------------------------------------

