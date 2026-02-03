#include "maths.h"

#include <cmath>
#include <cfloat>


//-------------------------------------------------------------------------------------------------

bool compute_factorial(double& val)
{
    if (val < 0)
        return false;

    const int ival = int(val);
    const double error = fabs(val - ival);
    if (error > FLT_EPSILON)
        return false;

    double newval = 1.0;
    for (int i = 2; i <= ival; ++i)
        newval *= i;

    val = newval;

    return true;
}

//-------------------------------------------------------------------------------------------------

double sinc(double v)
{
    double res = sin(v) / v;

    if (v == v && res != res)
        return 1.0;

    return res;
}

//-------------------------------------------------------------------------------------------------

