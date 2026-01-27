#include "libcalc.h"

#include "expr.h"
#include "parser.h"

#include <cstdio>


//-------------------------------------------------------------------------------------------------

void dtostr_human(double d, char* s, int sLen)
{
    snprintf(s, sLen, "    = %.8g", d);
}

//-------------------------------------------------------------------------------------------------

bool calc_eval(const char* expr, char* resBuffer, int resBufferLen)
{
    if (!resBuffer)
        return false;
    *resBuffer = 0;

    ParseCtx parseCtx { .InBuffer=expr, .ResBuffer=resBuffer, .ResBufferLen=resBufferLen };
    advance_token(parseCtx);

    double result = parse_expression(parseCtx);

    if (!accept(parseCtx, Token::Eof))
        on_parse_error(parseCtx, "trailing nonsense");
    
    if (parseCtx.Error)
        return false;

    dtostr_human(result, resBuffer, resBufferLen);

    return true;
}



