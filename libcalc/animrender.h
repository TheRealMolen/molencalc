#pragma once

#include "platform.h"
#include "plot.h"

#include <cstdint>

#if MLN_TARGET_PC
#include <SDL.h>
#endif


//-------------------------------------------------------------------------------------------------

// a little "oscilloscope" frame buffer
// uses 16-bit colours with a scopey palette
class TinyScopeFrameBuf
{
public:
    static constexpr int BORDER = 4;

    static constexpr int FULLWIDTH = 320;
    static constexpr int FULLHEIGHT = 320;

    static constexpr int IMGW = FULLWIDTH - 2*BORDER;
    static constexpr int IMGH = FULLHEIGHT - 2*BORDER;

    static const uint16_t kPalette[16];


    TinyScopeFrameBuf();


    void plot(int x, int y)
    {
        if (x < 0 || x >= IMGW)
            return;
        if (y < 0 || y >= IMGH)
            return;

        uint8_t* ppix = mPix + (y*ROWPITCH_BYTES + x/PIXELS_PER_BYTE);

        if (x & 1)  // low nybble
            *ppix |= 0x0f;
        else
            *ppix |= 0xf0;
    };

    // tick the screen so it darkens one step
    void tick();

    // output one row as renderable pixels
    void getRow(int y, uint16_t* rowBuf) const;


private:
    static constexpr int PIXELS_PER_BYTE = 2;
    static constexpr int ROWPITCH_BYTES = IMGW / PIXELS_PER_BYTE;

    uint8_t mPix[ROWPITCH_BYTES * IMGH];
    
};

//-------------------------------------------------------------------------------------------------

class AnimRenderer
{
    static constexpr int IMGW = TinyScopeFrameBuf::IMGW;
    static constexpr int IMGH = TinyScopeFrameBuf::IMGH;

    AnimRenderer() = delete;
    AnimRenderer(const AnimRenderer&) = delete;
    AnimRenderer(AnimRenderer&&) = delete;
    AnimRenderer& operator=(const AnimRenderer&) = delete;
    AnimRenderer& operator=(AnimRenderer&&) = delete;

public:
    AnimRenderer(float minX, float maxX, float minY, float maxY);
    ~AnimRenderer();

    void safePlot(int x, int y)
    {
        mFb.plot(x, y);
    };

    void fill(uint16_t col);
    
    void darken();

    void blit() const;

    bool check_for_break();

    inline int x(double realX) const { return int(mX.ToScreen(realX)); }
    inline int y(double realY) const { return int(mY.ToScreen(realY)); }

private:
    TinyScopeFrameBuf mFb;

#if MLN_TARGET_PC
    SDL_Surface* mSurf = nullptr;
#endif

    const PlotAxis mAxisX, mAxisY;
    const FastAxis mX, mY;
};

//-------------------------------------------------------------------------------------------------



