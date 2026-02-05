#include "expr.h"

#include "funcs.h"
#include "maths.h"
#include "parser.h"
#include "symbols.h"

#include <cmath>
#include <cstdio>
#include <cstring>

//-------------------------------------------------------------------------------------------------

// primary = number | "(" expression ")"
double parse_primary(ParseCtx& ctx)
{
    if (accept(ctx, Token::LParen))
    {
        double val = parse_expression(ctx);
        expect(ctx, Token::RParen);
        return val;
    }

    if (accept(ctx, Token::Minus))
    {
        return -expect_number(ctx);
    }

    return expect_number(ctx);
}

// postfix ::= primary | primary "!" | symbol "(" expression ")" | symbol
double parse_postfix(ParseCtx& ctx)
{
    char errBuf[20+kMaxSymbolLength+1];

    if (peek(ctx, Token::Symbol))
    {
        const int sym_name_pos = ctx.CurrIx;

        char symbol[kMaxSymbolLength];
        strcpy(symbol, ctx.TokenSymbol);
        expect(ctx, Token::Symbol);

        // if this is a (, we have a fn call. else it's a named value
        if (accept(ctx, Token::LParen))
        {
            double arg1 = parse_expression(ctx);
            if (!expect(ctx, Token::RParen))
                return 0.0;

            double val;
            if (eval_function(symbol, arg1, val, ctx))
                return val;

            if (ctx.Error)
                return 0.0;

            ctx.CurrIx = sym_name_pos;
            sprintf(errBuf, "unknown func: %s", symbol);
            on_parse_error(ctx, errBuf);
            return 0.0;
        }
        else
        {
            double val;
            if (eval_named_value(symbol, val))
                return val;

            ctx.CurrIx = sym_name_pos;
            sprintf(errBuf, "unknown named val: %s", symbol);
            on_parse_error(ctx, errBuf);
            return 0.0;
        }
    }

    double val = parse_primary(ctx);

    if (accept(ctx, Token::Factorial))
    {
        if (!compute_factorial(val))
        {
            on_parse_error(ctx, "need a positive integer");
            return 0.0;
        }
    }

    return val;
}

// exponent ::= postfix [ "**" postfix ]
double parse_exponent(ParseCtx& ctx)
{
    double val = parse_postfix(ctx);
    if (ctx.Error)
        return 0;

    if (accept(ctx, Token::Exponent))
    {
        double power = parse_postfix(ctx);
        if (ctx.Error)
            return 0;

        val = std::pow(val, power);
    }

    return val;
}

// unary = exponent | "+" unary | "-" unary
double parse_unary(ParseCtx& ctx)
{
    if (accept(ctx, Token::Plus))
        return parse_unary(ctx);
    if (accept(ctx, Token::Minus))
        return -1.0 * parse_unary(ctx);

    return parse_exponent(ctx);
}

// mul ::= unary | mul "*" unary | mul "/" unary | unary mul
double parse_mul(ParseCtx& ctx)
{
    bool allowed_implicit_mul = peek(ctx, Token::Number) || peek(ctx, Token::LParen);

    double val = parse_unary(ctx);

    bool had_infix = false;
    while (!ctx.Error && (peek(ctx, Token::Times) || peek(ctx, Token::Divide)))
    {
        had_infix = true;

        if (accept(ctx, Token::Times))
        {
            val = val * parse_unary(ctx);
        }
        else if (accept(ctx, Token::Divide))
        {
            val = val / parse_unary(ctx);
        }
    }

    // 2pi / (1+4)(3sin(x)) case
    if (allowed_implicit_mul && !had_infix)
    {
        if (peek(ctx, Token::Symbol) || peek(ctx, Token::LParen))
        {
            val *= parse_mul(ctx);
        }
    }

    return val;
}

// add ::= mul | add "+" mul | add "-" mul
double parse_add(ParseCtx& ctx)
{
    double val = parse_mul(ctx);

    while (!ctx.Error && (peek(ctx, Token::Plus) || peek(ctx, Token::Minus)))
    {
        if (accept(ctx, Token::Plus))
        {
            val = val + parse_mul(ctx);
        }
        else if (accept(ctx, Token::Minus))
        {
            val = val - parse_mul(ctx);
        }
    }

    return val;
}


double parse_expression(ParseCtx& ctx)
{
    return parse_add(ctx);
}

//-------------------------------------------------------------------------------------------------

