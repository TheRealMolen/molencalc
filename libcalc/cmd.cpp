#include "cmd.h"

#include "format.h"
#include "funcs.h"
#include "parser.h"
#include "symbols.h"

#include <cstring>

//-------------------------------------------------------------------------------------------------

static CommandDef gCommands[kMaxCommands];
static int gRegisteredCommands = 0;

//-------------------------------------------------------------------------------------------------

bool cmd_help(ParseCtx& ctx)
{
    if (peek(ctx, Token::Symbol))
    {
        const CommandDef* cmd = lookup_command(ctx.TokenSymbol);
        if (!cmd)
        {
            on_parse_error(ctx, "unknown help topic");
            return false;
        }
        
        calc_puts(cmd->Help);
        calc_puts("\n   ");
        calc_puts(cmd->Usage);
        calc_puts("\n");

        return expect(ctx, Token::Symbol);
    }

    calc_puts("Type expression, press Enter\n\n");
    calc_puts("Define var / function with\n");
    calc_puts(" <name>[<var>] = <expr in var>\n");
    calc_puts("eg.  f[x] = sin(x^2)\n");
    calc_puts("eg.  theta = 2pi/3\n");
    calc_puts("\n([{ and }]) are interchangeable\n\n");

    const CommandDef* cmd = gCommands;
    for (int i=0; i<gRegisteredCommands; ++i, ++cmd)
    {
        calc_puts(cmd->Name);
        calc_puts(" -- ");
        calc_puts(cmd->Help);
        calc_puts("\n");
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

bool cmd_list(const char*)
{
    calc_puts("== builtin functions ==\n");

    for (BuiltinFunctionIt it = function_builtin_begin();
        it;
        it = function_next(it))
    {
        calc_puts(function_name(it));
        calc_puts("\t");
    }

    calc_puts("\n\n== builtin constants ==\n");
    for (BuiltinSymbolIt it = symbol_builtin_begin();
        it;
        it = symbol_next(it))
    {
        calc_puts(symbol_name(it));
        calc_puts("\t");
    }
    calc_puts("\n");

    if (UserFunctionIt it = function_user_begin())
    {
        calc_puts("\n== user-defined functions ==\n");

        for (; it; it = function_next(it))
        {
            calc_puts("  ");
            calc_puts(function_name(it));
            calc_puts("(...) = ");
            calc_puts(function_def(it));
            calc_puts("\n");
        }
    }

    if (UserSymbolIt it = symbol_user_begin())
    {
        calc_puts("\n== user-defined symbols ==\n");

        for (; it; it = symbol_next(it))
        {
            char val_str[32];
            dtostr_human(symbol_val(it), val_str, sizeof(val_str));
            val_str[sizeof(val_str)-1] = 0;

            calc_puts("  ");
            calc_puts(symbol_name(it));
            calc_puts(" = ");
            calc_puts(val_str);
            calc_puts("\n");
        }
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

void init_commands()
{
    gRegisteredCommands = 0;

    register_calc_cmd(cmd_help, "help", "help [command]", "shows help");
    register_calc_cmd(cmd_list, "list", "list", "lists definitions");
}

//-------------------------------------------------------------------------------------------------

void register_calc_cmd(calc_cmd_func func, const char* name, const char* usage, const char* help)
{
    if (gRegisteredCommands >= kMaxCommands)
        return;

    CommandDef* cmd = gCommands + gRegisteredCommands;
    cmd->Name = name;
    cmd->Usage = usage;
    cmd->Help = help;
    cmd->Func = func;
    cmd->PFunc = nullptr;

    ++gRegisteredCommands;
}

void register_calc_cmd(calc_cmd_parser_func func, const char* name, const char* usage, const char* help)
{
    if (gRegisteredCommands >= kMaxCommands)
        return;

    CommandDef* cmd = gCommands + gRegisteredCommands;
    cmd->Name = name;
    cmd->Usage = usage;
    cmd->Help = help;
    cmd->Func = nullptr;
    cmd->PFunc = func;

    ++gRegisteredCommands;
}

//-------------------------------------------------------------------------------------------------

const CommandDef* lookup_command(const char* name)
{
    const CommandDef* cmd = gCommands;
    for (int i=0; i<gRegisteredCommands; ++i, ++cmd)
    {
        if (strcmp(cmd->Name, name) == 0)
            return cmd;
    }

    return nullptr;
}

//-------------------------------------------------------------------------------------------------

