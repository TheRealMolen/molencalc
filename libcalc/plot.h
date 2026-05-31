#pragma once

#include "libcalc.h"

#include "maths.h"
#include "parser.h"

#include "drivers/colours.h"

#include <cstdint>

//-------------------------------------------------------------------------------------------------

#ifndef MC_PLOT_WIDTH
#define MC_PLOT_WIDTH   240
#endif

#ifndef MC_PLOT_HEIGHT
#define MC_PLOT_HEIGHT  (((MC_PLOT_WIDTH) * 3) / 4)
#endif



//-------------------------------------------------------------------------------------------------
// consumer api
//
struct Plot;

const Plot* get_plot(); // returns null if a plot hasn't been created since reset_plot()
void reset_plot();



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// internal api
//

struct PlotAxis
{
    char Name[kMaxSymbolLength+1];
    real_t Lo = -1;
    real_t Hi = 1;
};

//-------------------------------------------------------------------------------------------------

struct FastAxis
{
    PlotAxis Axis;
    int StartI, EndI;
    int LoI, HiI;
    float Range;
    float IRange;
    float RangeRecip;
    float UnitsPerPix;

    FastAxis() = default;

    FastAxis(const PlotAxis& axis, int startI, int endI)
        : Axis(axis)
        , StartI(startI)
        , EndI(endI)
        , LoI(startI < endI ? startI : endI)
        , HiI(endI > startI ? endI : startI)
        , Range(axis.Hi - axis.Lo)
        , IRange(endI - startI)
        , RangeRecip(1.0 / Range)
        , UnitsPerPix(Range / IRange)
    { /**/ }

    // chart coords are ints from 0 at left of axis to (HiI-1) at the right
    // screen coords are ints from 0 to screen buf size (MC_PLOT_*)

    real_t FromChart(int vi) const
    {
        return Axis.Lo + (vi * UnitsPerPix);
    }
    real_t FromScreen(int vi) const
    {
        return Axis.Lo + ((vi - LoI) * UnitsPerPix);
    }

    int ToScreen(real_t v) const
    {
        return (StartI + IRange * ((v - Axis.Lo) * RangeRecip));
    }
    int ToScreenClamped(real_t v) const
    {
        const int scr = ToScreen(v);
        if (scr < LoI)
            return LoI;
        if (scr > HiI)
            return HiI;
        return scr;
    }

    bool IsScreenValInArea(real_t scr) const
    {
        const int si = int(scr);
        return ((si >= LoI) && (si <= HiI));
    }

};

//-------------------------------------------------------------------------------------------------

static constexpr int kPlotMaxLines = 5;

static constexpr int kPlotMaxLegendLines = kPlotMaxLines + 1;
static constexpr int kPlotMaxLegendEntryLen = 15;

struct PlotLegend
{
    char Text[kPlotMaxLegendEntryLen+1];
    col_t Col;
};

struct Plot
{
    col_t Pixels[MC_PLOT_WIDTH * MC_PLOT_HEIGHT];

    FastAxis X, Y;

    int NumLines = 0;

    int NumLegendLines = 0;
    PlotLegend LegendLines[kPlotMaxLegendLines];
};

//-------------------------------------------------------------------------------------------------

bool draw_plot(const char* func_name, const PlotAxis* xAxis, const PlotAxis* yAxis, ParseCtx& ctx);
void add_legend_line(const char* text, col_t colour);

void append_to_plot(const char* func_name, ParseCtx& ctx);

col_t plot_get_line_col(int i);

//-------------------------------------------------------------------------------------------------

