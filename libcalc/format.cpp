#include "format.h"

#include <cstdio>
#include <cstring>

//-------------------------------------------------------------------------------------------------

void dtostr_human(double d, char* s, int sLen)
{
    snprintf(s, sLen, "%.8g", d);
    s[sLen-1] = 0;

    // trim off trailing zeroes
    char* dot = strchr(s, '.');
    if (!dot)
        return;

    char* e = strchr(s, 'e');
    if (e)
        return; // this is fiddly. TODO

    char* z = s + strlen(s) - 1;
    while (z >= dot && *z == '0')
    {
        *z = 0;
        --z;
    }
    if (z == dot)
        *z = 0;
}

//-------------------------------------------------------------------------------------------------

