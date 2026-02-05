#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------

#define MCALC_WELCOME   "molencalc v16       don't panic\n\n"

//-------------------------------------------------------------------------------------------------

// used to specify a printing function that the calc can use
typedef void (*calc_puts_func)(const char* str);

// print a string through the registered puts fn
void calc_puts(const char* str);

//-------------------------------------------------------------------------------------------------

typedef bool (calc_cmd_func)(const char* args);

void register_calc_cmd(calc_cmd_func func, const char* name, const char* usage, const char* help);

//-------------------------------------------------------------------------------------------------

void calc_init(calc_puts_func puts_func);
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


