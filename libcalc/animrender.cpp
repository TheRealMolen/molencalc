#include "animrender.h"

#include "platform.h"

#include "drivers/keyboard.h"
#include "drivers/lcd.h"
#include "drivers/text.h"

#include <cstring>

#if MLN_TARGET_PC
// defined in the main SDL wrapper
extern SDL_Window* gWindow;
extern bool handle_input();
extern void render();
#endif

//-------------------------------------------------------------------------------------------------

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


static constexpr uint16_t COL_AMBER = 0xff0a;
static constexpr uint16_t COL_AQUA = 0xb7f5;
static constexpr uint16_t COL_TinyScope = COL_AMBER;

const uint16_t TinyScopeFrameBuf::kPalette[16] =
{
    0,
    darkenTimes<14>(COL_TinyScope),
    darkenTimes<13>(COL_TinyScope),
    darkenTimes<12>(COL_TinyScope),
    darkenTimes<11>(COL_TinyScope),
    darkenTimes<10>(COL_TinyScope),
    darkenTimes< 9>(COL_TinyScope),
    darkenTimes< 8>(COL_TinyScope),
    darkenTimes< 7>(COL_TinyScope),
    darkenTimes< 6>(COL_TinyScope),
    darkenTimes< 5>(COL_TinyScope),
    darkenTimes< 4>(COL_TinyScope),
    darkenTimes< 3>(COL_TinyScope),
    darkenTimes< 2>(COL_TinyScope),
    darkenTimes< 1>(COL_TinyScope),
    darkenTimes< 0>(COL_TinyScope),
};

TinyScopeFrameBuf::TinyScopeFrameBuf()
{
    memset(mPix, 0, sizeof(mPix));
}


void TinyScopeFrameBuf::tick()
{
    const uint8_t* endPix = mPix + (IMGH * ROWPITCH_BYTES);
    for (uint8_t* ppix = mPix; ppix != endPix; ++ppix)
    {
        uint8_t pix = *ppix;
        if (pix)
        {
            uint8_t pix0 = pix & 0xf0;
            uint8_t pix1 = pix & 0x0f;
            if (pix0)
                pix0 -= 0x10;
            if (pix1)
                --pix1;
            *ppix = pix0 | pix1;
        }
    }
}


void TinyScopeFrameBuf::getRow(int y, uint16_t* rowBuf) const
{
    const uint8_t* ppix = mPix + (y * ROWPITCH_BYTES);
    const uint8_t* pixEnd = ppix + ROWPITCH_BYTES;

    uint16_t* outPix = rowBuf;

    for (; ppix != pixEnd; ++ppix)
    {
        const uint8_t pix = *ppix;
        *(outPix++) = kPalette[pix >> 4];
        *(outPix++) = kPalette[pix & 0xf];
    }
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

AnimRenderer::AnimRenderer(float minX, float maxX, float minY, float maxY)
    : mAxisX{ .Name = "x", .Lo = minX, .Hi = maxX }
    , mAxisY{ .Name = "y", .Lo = minY, .Hi = maxY }
    , mX( mAxisX, 0, IMGW - 1)
    , mY( mAxisY, IMGW - 1, 0)
{
#if MLN_TARGET_PC
    mSurf = SDL_CreateRGBSurfaceWithFormat(0, IMGW, IMGH, 16, SDL_PIXELFORMAT_RGB565);
    if (!mSurf)
        return;
#endif

    lcd_scroll_clear();
    cursor_enable(false);

}

AnimRenderer::~AnimRenderer()
{
#if MLN_TARGET_PC
    SDL_FreeSurface(mSurf);
    mSurf = nullptr;
#endif

    cursor_enable(true);
}

//-------------------------------------------------------------------------------------------------

void AnimRenderer::darken()
{
    mFb.tick();
}

void AnimRenderer::blit() const
{
    // expand row-by-row to local array and then blit each of those in turn
    uint16_t row[IMGW];
    const int x = TinyScopeFrameBuf::BORDER;
    int y = TinyScopeFrameBuf::BORDER;
    for (int i=0; i<IMGH; ++i, ++y)
    {
        mFb.getRow(i, row);

        lcd_blit(row, x, y, IMGW, 1);
    }

#ifdef MLN_TARGET_PC
    lcd_refresh(gWindow);
#endif
}

bool AnimRenderer::check_for_break()
{
#if defined(MLN_TARGET_PC)

    return handle_input() == false;

#elif defined(MLN_TARGET_PICO)

    return keyboard_key_available();

#endif
}

//-------------------------------------------------------------------------------------------------

