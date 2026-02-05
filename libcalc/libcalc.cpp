#include "libcalc.h"

#include "cmd.h"
#include "expr.h"
#include "funcs.h"
#include "parser.h"
#include "plot.h"
#include "symbols.h"

#include <cstdio>
#include <cstring>

//-------------------------------------------------------------------------------------------------

calc_puts_func calc_puts_fn;

void calc_puts(const char* str)
{
    if (calc_puts_fn)
    {
        calc_puts_fn(str);
    }
}

//-------------------------------------------------------------------------------------------------

void dtostr_human(double d, char* s, int sLen)
{
    snprintf(s, sLen, "  = %.8g", d);
}

//-------------------------------------------------------------------------------------------------

// f: x-> expression
// statement ::= symbol ":" symbol "->" expression
// todo: arg list as symbol_list
bool parse_statement(ParseCtx& ctx)
{
    char name[kMaxSymbolLength+1];
    char arg[kMaxSymbolLength+1];

    if (!expect_symbol(ctx, name))
        return false;
    if (!expect(ctx, Token::Assign))
        return false;
    if (!expect_symbol(ctx, arg))
        return false;
    if (!peek(ctx, Token::Map))
        return false;

    // the remainder of the expression becomes the registered implementation of function <name>
    ParseCtx innerCtx {
        .InBuffer = ctx.InBuffer + ctx.CurrIx,
        .ResBuffer = ctx.ResBuffer,
        .ResBufferLen = ctx.ResBufferLen
    };
    if (!define_function(name, arg, innerCtx))
        return false;

    // we've eaten all the rest of the input
    ctx.NextToken = Token::Eof;

    return true;
}

//-------------------------------------------------------------------------------------------------

// axis ::= expression "<" symbol "<" expression
bool parse_axis(ParseCtx& ctx, PlotAxis& axis)
{
    double lo = parse_expression(ctx);
    if (ctx.Error)
        return false;

    if (!expect(ctx, Token::LessThan))
        return false;
    if (!expect_symbol(ctx, axis.Name))
        return false;
    if (!expect(ctx, Token::LessThan))
        return false;

    double hi = parse_expression(ctx);
    if (ctx.Error)
        return false;

    axis.Lo = lo;
    axis.Hi = hi;
    return true;
}


// g f -pi<x<pi, -1<y<1
// cmd_graph ::= "g" symbol [axis ["," axis]]
bool cmd_graph_y(ParseCtx& ctx)
{
    char func_name[kMaxSymbolLength+1];
    if (!expect_symbol(ctx, func_name))
    {
        on_parse_error(ctx, "need user func name for y=f(x)");
        return false;
    }
    if (!is_user_func(func_name))
    {
        on_parse_error(ctx, "unknown user function");
        return false;
    }

    PlotAxis x { .Name = "x" };
    PlotAxis y { .Name = "y" };

    while (!peek(ctx, Token::Eof))
    {
        const int axisStartIx = ctx.CurrIx;

        PlotAxis axis;
        if (!parse_axis(ctx, axis))
            return false;

        if (strcmp(axis.Name, x.Name) == 0)
            x = axis;
        else if (strcmp(axis.Name, y.Name) == 0)
            y = axis;
        else
        {
            ctx.CurrIx = axisStartIx;
            on_parse_error(ctx, "unknown axis");
            return false;
        }

        if (!accept(ctx, Token::Comma))
            break;
    }

    if (!draw_plot(func_name, &x, &y, ctx))
        return false;

    return true;
}

//-------------------------------------------------------------------------------------------------

// let x=3
// cmd_let ::= "let" symbol "equals" expression
bool cmd_let(ParseCtx& ctx)
{
    char symbol[kMaxSymbolLength+1];
    if (!expect_symbol(ctx, symbol))
    {
        on_parse_error(ctx, "need symbol name");
        return false;
    }
    if (!expect(ctx, Token::Equals))
        return false;

    const double val = parse_expression(ctx);
    if (ctx.Error)
        return false;

    return define_value(symbol, val, ctx);
}

//-------------------------------------------------------------------------------------------------

bool try_parse_command(ParseCtx& ctx)
{
    if (!peek(ctx, Token::Symbol))
        return false;

    const CommandDef* cmd = lookup_command(ctx.TokenSymbol);
    if (!cmd)
        return false;

    // eat the command name symbol
    expect(ctx, Token::Symbol);

    if (cmd->Func)
        return cmd->Func(ctx.InBuffer + ctx.CurrIx);
    if (cmd->PFunc)
        return cmd->PFunc(ctx);

    on_parse_error(ctx, "corrupt command");
    return false;
}

//-------------------------------------------------------------------------------------------------

void calc_init(calc_puts_func puts_func)
{
    calc_puts_fn = puts_func;

    init_commands();

    register_calc_cmd(cmd_graph_y, "g", "g fn [lo<x<hi] [, lo<y<hi]", "graph of y=fn(x)");
    register_calc_cmd(cmd_let, "let", "let var=expr", "set <var> to <expr>");
}

//-------------------------------------------------------------------------------------------------

bool calc_eval(const char* expr, char* resBuffer, int resBufferLen)
{
    if (!resBuffer)
        return false;
    *resBuffer = 0;

    ParseCtx parseCtx { .InBuffer=expr, .ResBuffer=resBuffer, .ResBufferLen=resBufferLen };
    advance_token(parseCtx);

    // scan the expression to see if it's something unusual
    const bool isStatement = (strstr(expr, "->") != nullptr);

    bool shouldPrintResult = false;
    double result = 0.0;

    if (isStatement && parse_statement(parseCtx))
    {
        strcpy(resBuffer, "  ok.");
    }
    else if (try_parse_command(parseCtx))
    {
        // commands are expected to manage their own feedback
    }
    else
    {
        result = parse_expression(parseCtx);
        shouldPrintResult = !parseCtx.Error;
    }

    if (!accept(parseCtx, Token::Eof))
        on_parse_error(parseCtx, "trailing nonsense");
    
    if (shouldPrintResult)
        dtostr_human(result, resBuffer, resBufferLen);

    return !parseCtx.Error;
}



