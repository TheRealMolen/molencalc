#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------

bool calc_eval(const char* expr, char* resBuffer, int resBufferLen);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#ifndef MC_PLOT_WIDTH
#define MC_PLOT_WIDTH   240
#endif

#ifndef MC_PLOT_HEIGHT
#define MC_PLOT_HEIGHT  (((MC_PLOT_WIDTH) * 3) / 4)
#endif

typedef struct
{
    uint16_t Pixels[MC_PLOT_WIDTH * MC_PLOT_HEIGHT];
} Plot;


const Plot* get_plot(); // returns null if a plot hasn't been created since reset_plot()
void reset_plot();

//-------------------------------------------------------------------------------------------------

#ifdef __cplusplus
};
#endif


