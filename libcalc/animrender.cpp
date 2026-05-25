#include "animrender.h"

#include "platform.h"

#include "drivers/colours.h"
#include "drivers/keyboard.h"
#include "drivers/lcd.h"
#include "drivers/palette.h"
#include "drivers/text.h"

#include <cstring>

#if MLN_TARGET_PC
// defined in the main SDL wrapper
extern SDL_Window* gWindow;
extern bool handle_input(bool *outAnyInput);
extern void render();
#endif

//-------------------------------------------------------------------------------------------------

static const uint8_t kFbPalStart = PAL_FBGRADIENT;  // our colour 0 maps to this colour in the global palette

//-------------------------------------------------------------------------------------------------

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


void TinyScopeFrameBuf::getRow(int y, col_t* rowBuf) const
{
#ifndef LCD_USEPALETTE
    const Palette* pal = gfx_get_palette();
    if (!pal)
        return;
#endif

    const uint8_t* ppix = mPix + (y * ROWPITCH_BYTES);
    const uint8_t* pixEnd = ppix + ROWPITCH_BYTES;

    col_t* outPix = rowBuf;

    for (; ppix != pixEnd; ++ppix)
    {
        const uint8_t pix = *ppix;

        const int pix0 = (pix >> 4) + kFbPalStart;
        const int pix1 = (pix & 0xf) + kFbPalStart;

#ifdef LCD_USEPALETTE
        *(outPix++) = pix0;
        *(outPix++) = pix1;
#else
        *(outPix++) = pal->Cols[pix0];
        *(outPix++) = pal->Cols[pix1];
#endif
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
    lcd_scroll_clear();
    cursor_enable(false);

}

AnimRenderer::~AnimRenderer()
{
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
    col_t row[IMGW];
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

    bool anyInput = false;
    handle_input(&anyInput);
    return anyInput;

#elif defined(MLN_TARGET_PICO)

    return keyboard_key_available();

#endif
}

//-------------------------------------------------------------------------------------------------

