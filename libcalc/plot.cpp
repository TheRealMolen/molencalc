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

bool draw_plot(const char* func_name, const PlotAxis* xAxis, const PlotAxis* yAxis, ParseCtx& ctx)
{
    if (!func_name || !xAxis || !yAxis)
        return false;

    const UserFunction* func = lookup_user_func(func_name);
    if (!func)
        return false;

    double x = 0;
    double y = eval_user_func(func, x, ctx);
    (void)y;
    
    gActivePlot = &gPlot;

    return true;
}

//-------------------------------------------------------------------------------------------------




