#pragma once

#include "libcalc.h"

//-------------------------------------------------------------------------------------------------

struct ParseCtx;

//-------------------------------------------------------------------------------------------------

typedef bool (calc_cmd_parser_func)(ParseCtx& ctx);
constexpr int kMaxCommands = 24;

struct CommandDef
{
    const char* Name = nullptr;
    const char* Usage = nullptr;
    const char* Help = nullptr;

    calc_cmd_func* Func = nullptr;
    calc_cmd_parser_func* PFunc = nullptr;
};

//-------------------------------------------------------------------------------------------------

void init_commands();

void register_calc_cmd(calc_cmd_func func, const char* name, const char* usage, const char* help);
void register_calc_cmd(calc_cmd_parser_func func, const char* name, const char* usage, const char* help);

const CommandDef* lookup_command(const char* name);

//-------------------------------------------------------------------------------------------------

