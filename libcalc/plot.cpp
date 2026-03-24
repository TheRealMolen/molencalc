#include "plot.h"

#include "funcs.h"

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

//-------------------------------------------------------------------------------------------------

static inline void safePlot(int x, int y, uint16_t col)
{
    if (y >= 0 && y < MC_PLOT_HEIGHT)
    {
        gPlot.Pixels[y * MC_PLOT_WIDTH + x] = col;
    }
}

static void interpolateY(int startXi, int startYi, int endYi, uint16_t col)
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

static void plot_hline_fast(int x0, int y, int x1, uint16_t col)
{
    uint16_t* pix = gPlot.Pixels + x0 + (y*MC_PLOT_WIDTH);
    const uint16_t* pixEnd = pix + (x1 - x0 + 1);
    while (pix != pixEnd)
        *(pix++) = col;
}

static void plot_vline_fast(int x, int y0, int y1, uint16_t col)
{
    uint16_t* pix = gPlot.Pixels + x + (y0*MC_PLOT_WIDTH);
    const uint16_t* pixEnd = pix + (y1 - y0 + 1) * MC_PLOT_WIDTH;
    for (; pix != pixEnd; pix += MC_PLOT_WIDTH)
        *pix= col;
}


bool draw_plot(const char* func_name, const PlotAxis* xAxis, const PlotAxis* yAxis, ParseCtx& ctx)
{
    if (!func_name || !xAxis || !yAxis)
        return false;

    const UserFunction* func = lookup_user_func(func_name);
    if (!func)
        return false;

    constexpr int border = 4;
    const uint16_t bgCol = 0x1862;
    const uint16_t axisCol = 0x39c4;
    const uint16_t lineCol = 0xff0a;

    const FastAxis xAx(*xAxis, border, MC_PLOT_WIDTH - border - 1);
    const FastAxis yAx(*yAxis, MC_PLOT_HEIGHT - border - 1, border);

    // clear our plot pixels
    uint16_t* pix = gPlot.Pixels;
    uint16_t* pixEnd = pix + (MC_PLOT_WIDTH * MC_PLOT_HEIGHT);
    for (; pix != pixEnd; ++pix)
        *pix = bgCol;

    // draw some axes
    const int xZeroScr = int(yAx.ToScreenClamped(0));
    plot_hline_fast(xAx.LoI, int(xZeroScr), xAx.HiI, axisCol);

    const int yZeroScr = int(xAx.ToScreenClamped(0));
    plot_vline_fast(yZeroScr, yAx.LoI, yAx.HiI, axisCol);
    
    double lastY = eval_user_func(func, xAx.LoI, ctx);
    int lastYi = -1;

    for (int xi=xAx.LoI; xi<=xAx.HiI; ++xi)
    {
        const double x = xAx.FromScreen(xi);
        const double y = eval_user_func(func, x, ctx);

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
    
    gActivePlot = &gPlot;

    return true;
}

//-------------------------------------------------------------------------------------------------




