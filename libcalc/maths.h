#pragma once

#include <cmath>

//-------------------------------------------------------------------------------------------------

// if val can be called a positive integer, update it with its factorial
// otherwise return false
bool compute_factorial(double& val);

double sinc(double v);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

using real_t = float;

constexpr static double pi = 3.14159265358979323846264338327950288419716939937510;

static constexpr real_t pi_real = real_t(pi);

//-------------------------------------------------------------------------------------------------

inline float lerp(float a, float b, float t)
{
    return a + (b-a)*t;
}

inline float signum(float x)
{
    if (x > 0.f)
        return 1;
    if (x < 0.f)
        return -1;
    return 0;
}

// return r in range [0,2pi)
inline real_t clampRads(real_t r)
{
    r = fmodf(r, pi_real*2);
    if (r < 0)
        r += pi_real*2;

    return r;
}

// return r in range (-pi,pi]
inline real_t clampRadsSym(real_t r)
{
    /* og code, tweaked to remove a funciton call in an inner loop
    r = fmodf(r, pi_real*2);
    if (r <= -pi_real)
        r += pi_real*2;
    if (r > pi_real)
        r -= pi_real*2;
        */

    const real_t scaled = r * (1 / (pi_real*2));
    real_t frac = scaled - int(scaled);

    if (frac <= -0.5)
        frac += 1;
    else if (frac > 0.5)
        frac -= 1;

    return frac * pi_real * 2;
}

//-------------------------------------------------------------------------------------------------

