#include "libcalc.h"

#include "chaos.h"
#include "cmd.h"
#include "expr.h"
#include "format.h"
#include "funcs.h"
#include "parser.h"
#include "plot.h"
#include "symbols.h"

#include "drivers/gfx.h"
#include "drivers/palette.h"

#include <cmath>
#include <cstring>
#include <iostream>

//-------------------------------------------------------------------------------------------------
// P A L E T T E   B I Z
//

namespace
{
    // warmly darken an RGB565 colour by ~10%
    constexpr uint16_t darken(uint16_t c)
    {
        uint16_t r = c >> 11;
        uint16_t g = (c >> 5) & 0x3f;
        uint16_t b = c & 0x1f;

        r = (r * 15) / 16;
        g = (g * 29) / 32;
        b = (b * 14) / 16;

        return (r << 11) | (g << 5) | (b);
    }

    template<unsigned int count>
    constexpr uint16_t darkenTimes(uint16_t c)
    {
        return darkenTimes<count - 1>(darken(c));
    }
    template<>
    constexpr uint16_t darkenTimes<0>(uint16_t c)
    {
        return c;
    }

    // lighten an RGB565 colour by ~10%
    constexpr uint16_t lighten(uint16_t c)
    {
        uint16_t r = c >> 11;
        uint16_t g = (c >> 5) & 0x3f;
        uint16_t b = c & 0x1f;

        r = (r * 17) / 16;
        g = (g * 35) / 32;
        b = (b * 18) / 16;

        if (r > 0x1f) r = 0x1f;
        if (g > 0x1f) g = 0x1f;
        if (b > 0x1f) b = 0x1f;

        return (r << 11) | (g << 5) | (b);
    }

    template<unsigned int count>
    constexpr uint16_t lightenTimes(uint16_t c)
    {
        return lightenTimes<count - 1>(lighten(c));
    }
    template<>
    constexpr uint16_t lightenTimes<0>(uint16_t c)
    {
        return c;
    }

    static constexpr uint16_t COL_AMBER = RGB16(255,227, 57);
    static constexpr uint16_t COL_AQUA = 0xb7f5;
    static constexpr uint16_t COL_TinyScope_DarkMode = COL_AMBER;

    static constexpr uint16_t COL_TinyScope_LiteMode = RGB16( 35, 91,116);
}

static constexpr uint16_t gDarkModeCols[] =
{
    RGB16(  0,  0,  0),     // default background
    RGB16(255,227, 57),     // default foreground

    RGB16( 49, 49, 49),     // graph area
    RGB16( 98, 95,  6),     // graph axes

    RGB16(224, 78,121),     // graph line 2
    RGB16( 29,179, 71),     // graph line 3
    RGB16( 63,153,192),     // graph line 4
    RGB16(194, 95,246),     // graph line 5

    0, 0, 0, 0,             // reserved
    0, 0, 0, 0,

    // tinyscope colours - gradient black -> col
    0,
    darkenTimes<14>(COL_TinyScope_DarkMode),
    darkenTimes<13>(COL_TinyScope_DarkMode),
    darkenTimes<12>(COL_TinyScope_DarkMode),
    darkenTimes<11>(COL_TinyScope_DarkMode),
    darkenTimes<10>(COL_TinyScope_DarkMode),
    darkenTimes< 9>(COL_TinyScope_DarkMode),
    darkenTimes< 8>(COL_TinyScope_DarkMode),
    darkenTimes< 7>(COL_TinyScope_DarkMode),
    darkenTimes< 6>(COL_TinyScope_DarkMode),
    darkenTimes< 5>(COL_TinyScope_DarkMode),
    darkenTimes< 4>(COL_TinyScope_DarkMode),
    darkenTimes< 3>(COL_TinyScope_DarkMode),
    darkenTimes< 2>(COL_TinyScope_DarkMode),
    darkenTimes< 1>(COL_TinyScope_DarkMode),
    darkenTimes< 0>(COL_TinyScope_DarkMode),
};
static constexpr uint8_t kNumDarkModeCols = sizeof(gDarkModeCols) / sizeof(gDarkModeCols[0]);
static_assert(kNumDarkModeCols == 32);

static const Palette gPaletteDark { kNumDarkModeCols, gDarkModeCols };


static constexpr uint16_t gLiteModeCols[] =
{
    RGB16(200,216,208),     // default background
    RGB16( 26, 96,104),     // default foreground

    RGB16(176,192,184),     // graph area
    RGB16(128,144,136),     // graph axes

    RGB16(209, 92,127),     // graph line 2
    RGB16( 60,157, 87),     // graph line 3
    RGB16(210,133, 17),     // graph line 4
    RGB16(235,255,  0),     // graph line 5

    0, 0, 0, 0,             // reserved
    0, 0, 0, 0,

    // tinyscope colours
    0xffff,
    lightenTimes<14>(COL_TinyScope_LiteMode),
    lightenTimes<13>(COL_TinyScope_LiteMode),
    lightenTimes<12>(COL_TinyScope_LiteMode),
    lightenTimes<11>(COL_TinyScope_LiteMode),
    lightenTimes<10>(COL_TinyScope_LiteMode),
    lightenTimes< 9>(COL_TinyScope_LiteMode),
    lightenTimes< 8>(COL_TinyScope_LiteMode),
    lightenTimes< 7>(COL_TinyScope_LiteMode),
    lightenTimes< 6>(COL_TinyScope_LiteMode),
    lightenTimes< 5>(COL_TinyScope_LiteMode),
    lightenTimes< 4>(COL_TinyScope_LiteMode),
    lightenTimes< 3>(COL_TinyScope_LiteMode),
    lightenTimes< 2>(COL_TinyScope_LiteMode),
    lightenTimes< 1>(COL_TinyScope_LiteMode),
    lightenTimes< 0>(COL_TinyScope_LiteMode),
};
static constexpr uint8_t kNumLiteModeCols = sizeof(gLiteModeCols) / sizeof(gLiteModeCols[0]);
static_assert(kNumLiteModeCols == 32);

static const Palette gPaletteLite { kNumLiteModeCols, gLiteModeCols };

//-------------------------------------------------------------------------------------------------

const Palette* palette_get_lite()
{
    return &gPaletteLite;
}

const Palette* palette_get_dark()
{
    return &gPaletteDark;
}

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
//-------------------------------------------------------------------------------------------------

static bool cmd_theme(const char* args)
{
    if (!args)
        return false;

    if (args[0] == 'd')
        gfx_set_palette(&gPaletteDark);
    else if (args[0] == 'l')
        gfx_set_palette(&gPaletteLite);
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
    cmdbuf[prefixLen+1] = 0;
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



