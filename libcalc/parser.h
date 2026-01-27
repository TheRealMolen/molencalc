#pragma once


//-----------------------------------------------------------------------------------------------

constexpr int kMaxSymbolLength = 23;

//-----------------------------------------------------------------------------------------------

enum class Token
{
    Invalid, Eof,

    Number,
    Symbol,

    Plus, Minus,
    Times, Divide,
    Exponent,
    LParen, RParen,
};

//-----------------------------------------------------------------------------------------------

struct ParseCtx
{
    const char* InBuffer = nullptr;
    int CurrIx = 0;

    char* ResBuffer = nullptr;
    int ResBufferLen = 0;
    bool Error = false;

    Token NextToken = Token::Invalid;

    double TokenNumber = 0.f;
    char TokenSymbol[kMaxSymbolLength+1] = {0};
};

//-----------------------------------------------------------------------------------------------

bool accept(ParseCtx& ctx, Token t);
bool expect(ParseCtx& ctx, Token t);
double expect_number(ParseCtx& ctx);
bool peek(const ParseCtx& ctx, Token t);

void advance_token(ParseCtx& ctx);
void on_parse_error(ParseCtx& ctx, const char* msg);

//-----------------------------------------------------------------------------------------------

