#include "parser.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>

//-------------------------------------------------------------------------------------------------

const char* kTokenNames[] =
{
    "INV", "EOF",
    "NUM",
    "SYM",
    "+", "-",
    "*", "/",
    "^",
    "(",")",
    "!",
    "<",">",
    "=",
    ":", "->",
    ",",
};
static_assert((sizeof(kTokenNames) / sizeof(kTokenNames[0])) == size_t(Token::COUNT));

//-------------------------------------------------------------------------------------------------

void on_parse_error(ParseCtx& ctx, const char* msg)
{
    // only one error at a time pls
    if (ctx.Error)
        return;

    ctx.Error = true;

    if (!ctx.ResBuffer)
        return;

    char* resBufEnd = ctx.ResBuffer + (ctx.ResBufferLen - 1);
    char* resCurr = ctx.ResBuffer;
    if (msg)
    {
        for (const char* in=msg; *in && resCurr < resBufEnd; ++in, ++resCurr)
            *resCurr = *in;

        if (resCurr < resBufEnd)
            *(resCurr++) = '\n';
    }

#ifdef MC_MONOSPACE
    // adding a marker to the bad char is a problem with variable-width fonts
    for (const char* in=ctx.InBuffer; *in && resCurr < resBufEnd; ++in, ++resCurr)
        *resCurr = *in;
    if (resCurr < resBufEnd)
        *(resCurr++) = '\n';

    for (int i=0; i<ctx.CurrIx && resCurr < resBufEnd; ++i, ++resCurr)
        *resCurr = ' ';
    if (resCurr < resBufEnd)
        *(resCurr++) = '^';
    if (resCurr < resBufEnd)
        *(resCurr++) = '\n';
#else
    {
        char nearBuf[16];
        strcpy(nearBuf, "  near '");

        char* to = nearBuf + strlen(nearBuf);

        const int startIx = std::max(0, ctx.CurrIx-1);
        const char* from = ctx.InBuffer + startIx;
        for (int ix = 0; ix+1 < 6 && *from; ++ix, ++from, ++to)
            *to = *from;
        *(to++) = '\'';
        *to = 0;

        for (const char* in=nearBuf; *in && resCurr < resBufEnd; ++in, ++resCurr)
            *resCurr = *in;
        if (resCurr < resBufEnd)
            *(resCurr++) = '\n';
    }
#endif

    *resCurr = 0;
    *resBufEnd = 0;
}

//-------------------------------------------------------------------------------------------------

void parse_number(ParseCtx& ctx)
{
    const char* start = ctx.InBuffer + ctx.CurrIx;
    char* end = nullptr;

    errno = 0;
    ctx.TokenNumber = strtod(start, &end);
    ctx.NextToken = Token::Number;

    if (end)
        ctx.CurrIx = end - ctx.InBuffer;

    if (errno)
    {
        on_parse_error(ctx, "scary number");
        ctx.NextToken = Token::Invalid;
        return;
    }

    // handle scale units
    const char c = ctx.InBuffer[ctx.CurrIx];
    switch (c)
    {
    case 'G':   ctx.TokenNumber *= 1.0e9;     break;
    case 'M':   ctx.TokenNumber *= 1.0e6;     break;
    case 'k':   ctx.TokenNumber *= 1.0e3;     break;
    case 'm':   ctx.TokenNumber *= 1.0e-3;    break;
    case 'u':   ctx.TokenNumber *= 1.0e-6;    break;
    case 'n':   ctx.TokenNumber *= 1.0e-9;    break;
    case 'p':   ctx.TokenNumber *= 1.0e-12;   break;
    default:
        return; // no suffix; don't increment CurrIx
    }

    ++ctx.CurrIx;
}

//-------------------------------------------------------------------------------------------------

inline bool is_symbol_char(char c, bool leading)
{
    if ((c >= 'A' && c <= 'Z') || ((c >= 'a') && (c <= 'z')))
        return true;
    if (c == '_')
        return true;

    if (leading)
        return false;

    if (c >= '0' && c <= '9')
        return true;

    return false;
}

char to_lower_sym(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (c + ('a' - 'A'));
    return c;
}

void parse_symbol(ParseCtx& ctx)
{
    const char* in = ctx.InBuffer + ctx.CurrIx;
    char* out = ctx.TokenSymbol;
    const char* outEnd = ctx.TokenSymbol + kMaxSymbolLength - 1;

    *(out++) = *(in++);
    while (out < outEnd && is_symbol_char(*in, false))
        *(out++) = to_lower_sym(*(in++));

    *out = 0;
    
    if (is_symbol_char(*in, false))
    {
        on_parse_error(ctx, "symbol too long");
        return;
    }

    ctx.CurrIx = (in - ctx.InBuffer);
    ctx.NextToken = Token::Symbol;
}

//-------------------------------------------------------------------------------------------------

void skip_whitespace(ParseCtx& ctx)
{
    const char* in = ctx.InBuffer + ctx.CurrIx;
    for (; *in == ' ' || *in == '\t'; ++in, ++ctx.CurrIx)
    {
        /**/
    }
}

void advance_token(ParseCtx& ctx)
{
    skip_whitespace(ctx);

    const char c = ctx.InBuffer[ctx.CurrIx];

    switch (c)
    {
    case 0:
        ctx.NextToken = Token::Eof;
        return; // nb. return early so we don't increment currIx

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case '.':
        parse_number(ctx);
        return; // nb. return early so we don't increment currIx again

    case '+': ctx.NextToken = Token::Plus;      break;
    case '*': ctx.NextToken = Token::Times;     break;
    case '^': ctx.NextToken = Token::Exponent;  break;
    case '/': ctx.NextToken = Token::Divide;    break;
    case '(': ctx.NextToken = Token::LParen;    break;
    case ')': ctx.NextToken = Token::RParen;    break;
    case '<': ctx.NextToken = Token::LessThan;  break;
    case '>': ctx.NextToken = Token::GreaterThan;  break;

    case '!': ctx.NextToken = Token::Factorial; break;
    case ':': ctx.NextToken = Token::Assign;    break;
    case '=': ctx.NextToken = Token::Equals;    break;
    case ',': ctx.NextToken = Token::Comma;     break;

    case '-':
    {
        const char nextc = ctx.InBuffer[ctx.CurrIx+1];
        if (nextc == '>')
        {
            ctx.NextToken = Token::Map;
            ctx.CurrIx += 2;
            return;
        }
        ctx.NextToken = Token::Minus;
        break;
    }

    default:
        if (is_symbol_char(c, true))
        {
            parse_symbol(ctx);
            return; // nb. return early so we don't increment currIx again
        }

        ctx.NextToken = Token::Invalid;
        return;
    }

    ++ctx.CurrIx;
}

//-----------------------------------------------------------------------------------------------

bool accept(ParseCtx& ctx, Token t)
{
    if (ctx.NextToken == t)
    {
        advance_token(ctx);
        return true;
    }
    return false;
}

bool expect(ParseCtx& ctx, Token t)
{
    if (ctx.Error)
        return false;

    if (accept(ctx, t))
        return true;

    char msg[256];
    sprintf(msg, "unexpected token. expected %s", kTokenNames[int(t)]);
    on_parse_error(ctx, msg);
    return false;
}

double expect_number(ParseCtx& ctx)
{
    if (ctx.Error)
        return 0.0;

    if (ctx.NextToken == Token::Number)
    {
        double val = ctx.TokenNumber;
        advance_token(ctx);
        return val;
    }

    on_parse_error(ctx, "expected number");
    return 0;
}

bool expect_symbol(ParseCtx& ctx, char* outSymbolBuf)  // outSymbolBuf must be at least kMaxSymbolLength+1 long
{
    if (ctx.Error)
        return false;

    if (ctx.NextToken == Token::Symbol)
    {
        strcpy(outSymbolBuf, ctx.TokenSymbol);
        advance_token(ctx);
        return true;
    }

    on_parse_error(ctx, "expected symbol");
    return false;
}

bool peek(const ParseCtx& ctx, Token t)
{
    if (ctx.Error)
        return false;

    return ctx.NextToken == t;
}

//-------------------------------------------------------------------------------------------------


