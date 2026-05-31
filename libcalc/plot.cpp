#include "plot.h"

#include "drivers/colours.h"
#include "drivers/lcd.h"
#include "drivers/palette.h"

#include "funcs.h"

//-------------------------------------------------------------------------------------------------

static const col8_t kLineColours[kPlotMaxLines] = 
{
    PAL_FG,
    PAL_PLOTCOLS + 0,
    PAL_PLOTCOLS + 1,
    PAL_PLOTCOLS + 2,
    PAL_PLOTCOLS + 3,
};
static_assert((sizeof(kLineColours) / sizeof(kLineColours[0])) == kPlotMaxLines);

//-------------------------------------------------------------------------------------------------

static Plot gPlot;
static Plot* gActivePlot = nullptr;

//-------------------------------------------------------------------------------------------------

const Plot* get_plot()
{
    return gActivePlot;
}

void reset_plot()
{
    gActivePlot = nullptr;
}

col_t plot_get_line_col(int i)
{
    return kLineColours[i % kPlotMaxLines];
}

//-------------------------------------------------------------------------------------------------

static inline void safePlot(int x, int y, col_t col)
{
    if (y >= 0 && y < MC_PLOT_HEIGHT)
    {
        gPlot.Pixels[y * MC_PLOT_WIDTH + x] = col;
    }
}

static void interpolateY(int startXi, int startYi, int endYi, col_t col)
{
    if (startYi > endYi)
    {
        // asymptote
        if (startYi - endYi > MC_PLOT_HEIGHT)
            return;

        if (startYi > MC_PLOT_HEIGHT-1)
            startYi = MC_PLOT_HEIGHT-1;
        if (endYi < 0)
            endYi = 0;
        if (startYi <= endYi)
            return;

        const int midYi = (startYi + endYi) / 2;
        for (int yi = startYi-1; yi > midYi; --yi)
            safePlot(startXi, yi, col);
        for (int yi = midYi; yi > endYi; --yi)
            safePlot(startXi+1, yi, col);
    }
    else
    {
        // asymptote
        if (startYi - endYi < -MC_PLOT_HEIGHT)
            return;

        if (startYi < 0)
            startYi = 0;
        if (endYi > MC_PLOT_HEIGHT-1)
            endYi = MC_PLOT_HEIGHT-1;
        if (startYi >= endYi)
            return;

        const int midYi = (startYi + endYi) / 2;
        for (int yi = startYi+1; yi < midYi; ++yi)
            safePlot(startXi, yi, col);
        for (int yi = midYi; yi < endYi; ++yi)
            safePlot(startXi+1, yi, col);
    }
};

[[maybe_unused]] static void plot_hline_fast(int x0, int y, int x1, col_t col)
{
    col_t* pix = gPlot.Pixels + x0 + (y*MC_PLOT_WIDTH);
    const col_t* pixEnd = pix + (x1 - x0 + 1);
    while (pix != pixEnd)
        *(pix++) = col;
}

[[maybe_unused]] static void plot_vline_fast(int x, int y0, int y1, col_t col)
{
    col_t* pix = gPlot.Pixels + x + (y0*MC_PLOT_WIDTH);
    const col_t* pixEnd = pix + (y1 - y0 + 1) * MC_PLOT_WIDTH;
    for (; pix != pixEnd; pix += MC_PLOT_WIDTH)
        *pix= col;
}

static void plot_hline_pat(int x0, int y, int x1, col_t col, uint8_t pattern)
{
    col_t* pix = gPlot.Pixels + x0 + (y*MC_PLOT_WIDTH);
    const col_t* pixEnd = pix + (x1 - x0 + 1);
    for (; pix != pixEnd; ++pix)
    {
        if (pattern & 0x80)
            *pix = col;

        pattern = (pattern << 1) | (pattern >> 7);
    }
}

static void plot_vline_pat(int x, int y0, int y1, col_t col, uint8_t pattern)
{
    col_t* pix = gPlot.Pixels + x + (y0*MC_PLOT_WIDTH);
    const col_t* pixEnd = pix + (y1 - y0 + 1) * MC_PLOT_WIDTH;
    for (; pix != pixEnd; pix += MC_PLOT_WIDTH)
    {
        if (pattern & 0x80)
            *pix= col;

        pattern = (pattern << 1) | (pattern >> 7);
    }
}


bool draw_plot(const char* func_name, const PlotAxis* xAxis, const PlotAxis* yAxis, ParseCtx& ctx)
{
    if (!func_name || !xAxis || !yAxis)
        return false;

    const UserFunction* func = lookup_user_func(func_name);
    if (!func)
        return false;

    gPlot.NumLines = 0;
    gPlot.NumLegendLines = 0;

    constexpr int border = 4;
    gPlot.X = {*xAxis, border, MC_PLOT_WIDTH - border - 1};
    gPlot.Y = {*yAxis, MC_PLOT_HEIGHT - border - 1, border};
    const FastAxis& xAx = gPlot.X;
    const FastAxis& yAx = gPlot.Y;

#if LCD_USEPALETTE
    const col_t bgCol = PAL_PLOTAREA;
    const col_t axisCol = PAL_PLOTAXIS;
    const col_t lineCol = kLineColours[gPlot.NumLines];
#else
    const Palette* pal = gfx_get_palette();
    const col_t bgCol = pal->Cols[PAL_PLOTAREA];
    const col_t axisCol = pal->Cols[PAL_PLOTAXIS];
    const col_t lineCol = pal->Cols[kLineColours[gPlot.NumLines]];
#endif

    // clear our plot pixels
    col_t* pix = gPlot.Pixels;
    col_t* pixEnd = pix + (MC_PLOT_WIDTH * MC_PLOT_HEIGHT);
    for (; pix != pixEnd; ++pix)
        *pix = bgCol;

    // draw some axes
    const int xZeroScr = int(yAx.ToScreenClamped(0));
    plot_hline_pat(xAx.LoI, int(xZeroScr), xAx.HiI, axisCol, 0x55);

    const int yZeroScr = int(xAx.ToScreenClamped(0));
    plot_vline_pat(yZeroScr, yAx.LoI, yAx.HiI, axisCol, 0x55);
    
    double lastY = eval_user_func(func, xAx.LoI, ctx);
    int lastYi = -1;

    for (int xi=xAx.LoI; xi<=xAx.HiI; ++xi)
    {
        const double x = xAx.FromScreen(xi);
        const double y = eval_user_func(func, x, ctx);
        if (ctx.Error)
            return false;

        const double yscr = yAx.ToScreen(y);
        const int yi = int(yscr);

        if (y == y)
        {
            safePlot(xi, yi, lineCol);

            // interpolate if needed and if no nans
            if (xi > xAx.LoI && lastY==lastY)
            {
                const int deltaYi = yi - lastYi;
                if (deltaYi > 1 || deltaYi < -1)
                   interpolateY(xi - 1, lastYi, yi, lineCol);
            }
        }

        lastY = y;
        lastYi = yi;
    }

    gPlot.NumLines = 1;
    
    gActivePlot = &gPlot;

    return true;
}

//-------------------------------------------------------------------------------------------------

void append_to_plot(const char* func_name, ParseCtx& ctx)
{
    if (ctx.Error || !func_name || !gActivePlot)
        return;
    if (gActivePlot->NumLines >= kPlotMaxLines)
        return;

    const UserFunction* func = lookup_user_func(func_name);
    if (!func)
        return;

    const FastAxis& xAx = gPlot.X;
    const FastAxis& yAx = gPlot.Y;

#if LCD_USEPALETTE
    const col_t lineCol = kLineColours[gPlot.NumLines];
#else
    const Palette* pal = gfx_get_palette();
    const col_t lineCol = pal->Cols[kLineColours[gPlot.NumLines]];
#endif

    double lastY = eval_user_func(func, xAx.LoI, ctx);
    int lastYi = -1;

    for (int xi=xAx.LoI; xi<=xAx.HiI; ++xi)
    {
        const double x = xAx.FromScreen(xi);
        const double y = eval_user_func(func, x, ctx);
        if (ctx.Error)
            return;

        const double yscr = yAx.ToScreen(y);
        const int yi = int(yscr);

        if (y == y)
        {
            safePlot(xi, yi, lineCol);

            // interpolate if needed and if no nans
            if (xi > xAx.LoI && lastY==lastY)
            {
                const int deltaYi = yi - lastYi;
                if (deltaYi > 1 || deltaYi < -1)
                   interpolateY(xi - 1, lastYi, yi, lineCol);
            }
        }

        lastY = y;
        lastYi = yi;
    }

    ++gPlot.NumLines;
}

//-------------------------------------------------------------------------------------------------

void strcpy_ellipsis(char* dest, int destBufSize, const char* src)
{
    int safeLen = destBufSize - 4;
    if (!dest)
        return;
    if (!src || safeLen <= 0)
    {
        *dest = 0;
        return;
    }
        
    const char* srcSafeEnd = src + safeLen;

    const char* s = src;
    char* d = dest;
    for (; *s && s != srcSafeEnd; ++s, ++d)
        *d = *s;

    // we may need to ellipsify...
    if (s[0] && s[1] && s[2] && s[3])
    {
        d[0] = '.';
        d[1] = '.';
        d[2] = '.';
        d[3] = 0;
        return;
    }

    char* destEnd = dest + (destBufSize - 1);
    for (; *s && d != destEnd; ++s, ++d)
        *d = *s;

    *d = 0;
}

//-------------------------------------------------------------------------------------------------

void add_legend_line(const char* text, col_t colour)
{
    if (!gActivePlot || gActivePlot->NumLegendLines >= kPlotMaxLegendLines)
        return;

    PlotLegend& leg = gActivePlot->LegendLines[gActivePlot->NumLegendLines];
    ++gActivePlot->NumLegendLines;

    leg.Col = colour;
    strcpy_ellipsis(leg.Text, sizeof(leg.Text), text);
}

//-------------------------------------------------------------------------------------------------

#if MLN_UNIT_TESTS
#include "extern/doctest.h"
#include <string>

TEST_CASE("strcpy_ellipsis")
{
    char outBuf[10];

    {
        std::string empty;
        strcpy_ellipsis(outBuf, sizeof(outBuf), empty.c_str());
        CHECK_EQ(empty, outBuf);
    }
    {
        std::string maxLen("123456789");
        strcpy_ellipsis(outBuf, sizeof(outBuf), maxLen.c_str());
        CHECK_EQ(maxLen, outBuf);
    }
    {
        std::string longStr("When shall we three meet again?");
        strcpy_ellipsis(outBuf, sizeof(outBuf), longStr.c_str());
        CHECK_NE(longStr, outBuf);
        std::string longStringEllipse("When s...");
        CHECK_EQ(longStringEllipse, outBuf);
    }
    {
        std::string minTrunc("1234567890");
        strcpy_ellipsis(outBuf, sizeof(outBuf), minTrunc.c_str());
        CHECK_NE(minTrunc, outBuf);
        std::string minTruncEllipse("123456...");
        CHECK_EQ(minTruncEllipse, outBuf);
    }
}



#endif

//-------------------------------------------------------------------------------------------------

