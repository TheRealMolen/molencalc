#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "drivers/colours.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------

#define MCALC_WELCOME   "molencalc v21\t   don't panic\n"

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

void calc_process_input();

//-------------------------------------------------------------------------------------------------

#ifdef __cplusplus
};
#endif


