#pragma once

#include "libcalc.h"

#include "maths.h"
#include "parser.h"

#include <stdint.h>

//-------------------------------------------------------------------------------------------------

struct PlotAxis
{
    char Name[kMaxSymbolLength+1];
    real_t Lo = -1;
    real_t Hi = 1;
};

//-------------------------------------------------------------------------------------------------

struct FastAxis
{
    const PlotAxis& Axis;
    int StartI, EndI;
    int LoI, HiI;
    float Range;
    float IRange;
    float RangeRecip;
    float UnitsPerPix;

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

bool draw_plot(const char* func_name, const PlotAxis* xAxis, const PlotAxis* yAxis, ParseCtx& ctx);

//-------------------------------------------------------------------------------------------------

