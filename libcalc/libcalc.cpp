#include "libcalc.h"

#include "cmd.h"
#include "expr.h"
#include "format.h"
#include "funcs.h"
#include "parser.h"
#include "plot.h"
#include "symbols.h"

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

// assignment ::= "->" | "="
// f[x] assignment expression
// x assignment expression
// definition ::= symbol [lparen symbol rparen] assignment expression
bool parse_definition(ParseCtx& ctx)
{
    char name[kMaxSymbolLength+1];

    bool isFunction = false;
    char arg[kMaxSymbolLength+1];

    if (!expect_symbol(ctx, name))
        return false;

    if (accept(ctx, Token::LParen))
    {
        isFunction = true;

        if (!expect_symbol(ctx, arg))
            return false;
        if (!expect(ctx, Token::RParen))
            return false;
    }

    // we need to cache the pointer to the rest of the string now before we advance token
    // otherwise the function def will miss the first token
    const char* postAssignBuf = ctx.InBuffer + ctx.CurrIx;

    if (!accept(ctx, Token::Map) && !expect(ctx, Token::Equals))
        return false;

    if (isFunction)
    {
        // the remainder of the expression becomes the registered implementation of function <name>
        ParseCtx innerCtx {
            .InBuffer = postAssignBuf,
            .ResBuffer = ctx.ResBuffer,
            .ResBufferLen = ctx.ResBufferLen
        };
        if (!define_function(name, arg, innerCtx))
            return false;

        // we've eaten all the rest of the input
        ctx.NextToken = Token::Eof;
    }
    else
    {
        const double val = parse_expression(ctx);
        if (ctx.Error)
            return false;

        return define_value(name, val, ctx);
    }

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
    const bool isDefinition = (strchr(expr, '=') != nullptr) || (strstr(expr, "->") != nullptr);

    bool shouldPrintResult = false;
    double result = 0.0;

    if (isDefinition && parse_definition(parseCtx))
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
    {
        strcpy(resBuffer, "  = ");
        const int introLen = strlen(resBuffer);
        dtostr_human(result, resBuffer + introLen, resBufferLen - introLen);
    }

    return !parseCtx.Error;
}



