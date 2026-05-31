#include "libcalc.h"

#include "chaos.h"
#include "cmd.h"
#include "expr.h"
#include "format.h"
#include "funcs.h"
#include "palette.h"
#include "parser.h"
#include "plot.h"
#include "symbols.h"

#include "drivers/font.h"
#include "drivers/gfx.h"
#include "drivers/palette.h"
#include "drivers/text.h"

#include <cmath>
#include <cstring>
#include <iostream>

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

//-------------------------------------------------------------------------------------------------

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
    {
        reset_plot();
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

static constexpr int kMaxElementsInVarList = 5; // that's as many colours as we have in the palette

// primary_list ::= primary | primary "," primary_list
// returns the number of vals parsed. outVals needs space for at least kMaxElementsInVarList
int parse_primary_list(ParseCtx& ctx, double* outVals, int maxVals)
{
    int numVals = 0;
    outVals[numVals] = parse_primary(ctx);
    if (ctx.Error)
        return 0;

    ++numVals;

    while (accept(ctx, Token::Comma))
    {
        if (numVals >= maxVals)
        {
            on_parse_error(ctx, "too many values in list");
            return 0;
        }

        outVals[numVals] = parse_primary(ctx);
        if (ctx.Error)
            return 0;

        ++numVals;
    }

    return numVals;
}



// mg f k=1,2,3 -pi<x<pi, -1<y<1
// cmd_multigraph ::= "mg" symbol symbol=val_list [axis ["," axis]]
bool cmd_multigraph_y(ParseCtx& ctx)
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

    char sym_name[kMaxSymbolLength+1];
    if (!expect_symbol(ctx, sym_name))
    {
        on_parse_error(ctx, "missing name of varying symbol");
        return false;
    }

    UserSymbolIt itSym = lookup_user_sym(sym_name);
    if (!itSym)
    {
        on_parse_error(ctx, "unknown symbol");
        return false;
    }
    if (!expect(ctx, Token::Equals))
        return false;
    double symVals[kMaxElementsInVarList];
    int numSymVals = parse_primary_list(ctx, symVals, kMaxElementsInVarList);
    if (ctx.Error)
        return false;
    if (numSymVals <= 1)
    {
        on_parse_error(ctx, "invalid/missing val list");
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

    set_user_sym(itSym, symVals[0]);
    if (!draw_plot(func_name, &x, &y, ctx))
    {
        reset_plot();
        return false;
    }

    char legend[49];
    snprintf(legend, sizeof(legend), "y=%s(x)", func_name);
    add_legend_line(legend, PAL_FG);

    auto add_legend = [&legend, sym_name](double val, int valIx)
    {
        char valStr[20];
        dtostr_human(val, valStr, sizeof(valStr));
        snprintf(legend, sizeof(legend), "%s=%s", sym_name, valStr);
        add_legend_line(legend, plot_get_line_col(valIx));
    };

    add_legend(symVals[0], 0);

    for (int valIx = 1; valIx < numSymVals; ++valIx)
    {
        double val = symVals[valIx];
        set_user_sym(itSym, val);

        append_to_plot(func_name, ctx);

        add_legend(symVals[valIx], valIx);
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static bool cmd_theme(const char* args)
{
    if (!args)
        return false;

    if (args[0] == 'd')
        gfx_set_palette(palette_get_dark());
    else if (args[0] == 'l')
        gfx_set_palette(palette_get_lite());
    else
        return false;

    return true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

bool try_parse_command(ParseCtx& ctx)
{
    if (!peek(ctx, Token::Symbol))
        return false;

    const CommandDef* cmd = lookup_command(ctx.TokenSymbol);
    if (!cmd)
        return false;

    // eat the command name symbol
    const int argsOffset = ctx.CurrIx;
    expect(ctx, Token::Symbol);

    if (cmd->Func)
    {
        // skip whitepace in the input
        const char* args = ctx.InBuffer + argsOffset; 
        while (*args == ' ')
            ++args;

        const bool result = cmd->Func(args);
        ctx.NextToken = Token::Eof;

        return result;
    }
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
    register_calc_cmd(cmd_multigraph_y, "mg", "mg fn V=a,b,... [lo<x<hi] [, lo<y<hi]", "multigraph of y=fn(x)");
    register_calc_cmd(cmd_theme, "theme", "theme dark|lite", "changes colour theme");

    register_chaos_commands();
}

//-------------------------------------------------------------------------------------------------

static bool is_expr_definition(const char* expr)
{
    const char* eqPtr = strchr(expr, '=');
    const char* mapPtr = strstr(expr, "->");
    if (!eqPtr && !mapPtr)
        return false;

    const char* assignmentPtr;
    if (eqPtr && mapPtr)
        assignmentPtr = std::min(eqPtr, mapPtr);
    else
        assignmentPtr = std::max(eqPtr, mapPtr);

    const char* spacePtr = strchr(expr, ' ');
    if (!spacePtr)
        return true;

    if (assignmentPtr < spacePtr)
        return true;

    // we now need to disambiguate "f(x) = ..." and "xy a=3"
    const int prefixLen = spacePtr - expr;
    char cmdbuf[kMaxSymbolLength+1];
    memcpy(cmdbuf, expr, prefixLen);
    cmdbuf[prefixLen] = 0;
    const CommandDef* cmd = lookup_command(cmdbuf);
    if (cmd)
        return false;

    return true;
}

//-------------------------------------------------------------------------------------------------

bool calc_eval(const char* expr, char* resBuffer, int resBufferLen)
{
    if (!resBuffer)
        return false;
    *resBuffer = 0;
    if (!expr || !expr[0])
        return false;

    ParseCtx parseCtx { .InBuffer=expr, .ResBuffer=resBuffer, .ResBufferLen=resBufferLen };
    advance_token(parseCtx);

    // scan the expression to see if it's something unusual
    const bool isDefinition = is_expr_definition(expr);

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

//-------------------------------------------------------------------------------------------------

void calc_process_input()
{
    char resBuf[1024];

    reset_plot();

    calc_eval(input_get_line(), resBuf, sizeof(resBuf));

    text_emit_str(resBuf);

    if (const Plot* plot = get_plot())
    {
        text_put_image(plot->Pixels, MC_PLOT_WIDTH, MC_PLOT_HEIGHT);

        const Font* oldFont = text_get_font();
        const Font& font = font_5x10;
        text_set_font(&font);
        const col_t oldCol = text_get_foreground();

        int y = 20 - MC_PLOT_HEIGHT;
        for (int i=0; i<plot->NumLegendLines; ++i)
        {
            const PlotLegend& leg = plot->LegendLines[i];
            text_set_foreground(leg.Col);

            int x = 10;
            for (const char* c = leg.Text; *c; ++c)
                x += text_putc(x, y, *c);

             y += font.Height;
        }

        text_set_font(oldFont);
        text_set_foreground(oldCol);
    }

    text_emit_str("\n");
    input_reset_line();
}

//-------------------------------------------------------------------------------------------------

