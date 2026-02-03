#pragma once

#include "libcalc.h"
#include "parser.h"

#include <stdint.h>

//-------------------------------------------------------------------------------------------------

struct PlotAxis
{
    char Name[kMaxSymbolLength+1];
    double Lo = -1;
    double Hi = 1;
};

//-------------------------------------------------------------------------------------------------

const Plot* get_plot(); // returns null if a plot hasn't been created since reset_plot()
void reset_plot();

//-------------------------------------------------------------------------------------------------

bool draw_plot(const char* func_name, const PlotAxis* xAxis, const PlotAxis* yAxis, ParseCtx& ctx);

//-------------------------------------------------------------------------------------------------

