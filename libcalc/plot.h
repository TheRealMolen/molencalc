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

bool draw_plot(const char* func_name, const PlotAxis* xAxis, const PlotAxis* yAxis, ParseCtx& ctx);

//-------------------------------------------------------------------------------------------------

