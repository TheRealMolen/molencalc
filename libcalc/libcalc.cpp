#include "libcalc.h"

#include "expr.h"
#include "funcs.h"
#include "parser.h"
#include "plot.h"
#include "symbols.h"

#include <cstdio>
#include <cstring>

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


// :g f -pi<x<pi, -1<y<1
// cmd_graph ::= ":" "g" symbol [axis ["," axis]]
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

// :let x=3
// cmd_let ::= ":" "let" symbol "equals" expression
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

bool cmd_help(ParseCtx& ctx)
{
    const char* helpText =
R"(commands start with :
graph of y=f(x)
  :g fn [lo<x<hi] [lo<y<hi]
set named val
  :let x=expr
)";

    if (strlen(helpText) < size_t(ctx.ResBufferLen))
        strcpy(ctx.ResBuffer, helpText);
    else
        strcpy(ctx.ResBuffer, "error: too much help for buf");

    return true;
}

//-------------------------------------------------------------------------------------------------

bool parse_command(ParseCtx& ctx)
{
    if (!expect(ctx, Token::Assign))
        return false;

    char cmd[kMaxSymbolLength+1];
    if (!expect_symbol(ctx, cmd))
        return false;

    if (strcmp(cmd, "g") == 0)
        return cmd_graph_y(ctx);
    if (strcmp(cmd, "let") == 0)
        return cmd_let(ctx);
    if ((strcmp(cmd, "help") == 0) || (strcmp(cmd, "h") == 0))
        return cmd_help(ctx);

    on_parse_error(ctx, "unknown command");
    return false;
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
    const bool isCommand = expr && expr[0] == ':';
    const bool isStatement = (strstr(expr, "->") != nullptr);
    const bool isExpression = !isCommand && !isStatement;

    double result = 0.0;

    if (isStatement && parse_statement(parseCtx))
    {
        strcpy(resBuffer, "  ok.");
    }
    else if (isCommand)
    {
        // commands are expected to manage their own feedback
        parse_command(parseCtx);
    }
    else if (isExpression)
    {
        result = parse_expression(parseCtx);
    }

    if (!accept(parseCtx, Token::Eof))
        on_parse_error(parseCtx, "trailing nonsense");
    
    if (parseCtx.Error)
        return false;

    if (isExpression)
        dtostr_human(result, resBuffer, resBufferLen);

    return true;
}



