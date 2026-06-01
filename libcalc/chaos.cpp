#include "chaos.h"

#include "animrender.h"
#include "cmd.h"
#include "expr.h"
#include "maths.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// some chaotic systems from Elegant Chaos by Julien Clinton Sprott

struct DampedPendulumSystem
{
    real_t x = 0;
    real_t v = 1;
    real_t z = 0;
    
    real_t damp = 0.05;
    real_t omega = 0.8;

    void setParamA(double val)  { damp = real_t(val); }
    void setParamB(double val)  { omega = real_t(val); }

    real_t getX() const { return x; }
    real_t getY() const { return v; }
    real_t getPhi() const { return z; }

    void next(real_t dt)
    {
        z += dt * omega;
        v += dt * ((-damp * v) - sinf(x) + sinf(z));
        x += dt * v;

        if (z > pi_real*2)
            z -= pi_real*2;
        x = clampRadsSym(x);
    }
};


struct ForcedVdPolOscillator
{
    real_t x = 1;
    real_t v = 0.1;
    real_t z = 0;
    
    real_t force = 0.5;
    real_t omega = 0.1;

    void setParamA(double val)  { force = real_t(val); }
    void setParamB(double val)  { omega = real_t(val); }

    real_t getX() const { return x; }
    real_t getY() const { return v; }
    real_t getPhi() const { return z; }

    void next(real_t dt)
    {
        z += dt * omega;
        v += dt * ((force * sinf(z)) - x - ((x*x - 1) * v));
        x += dt * v;

        if (z > pi_real*2)
            z -= pi_real*2;
        x = clampRadsSym(x);
    }
};


struct SignumSystem
{
    real_t x = 1;
    real_t v = 0.1;
    real_t z = 0;
    
    void setParamA(double val)  { x = real_t(val); }
    void setParamB(double val)  { v = real_t(val); }

    real_t getX() const { return x * 0.6f; }
    real_t getY() const { return v; }
    real_t getPhi() const { return z; }

    void next(real_t dt)
    {
        z += dt;
        v += dt * (sinf(z) - signum(x));
//        v += dt * (sin(z) - tanh(x*5000));
        x += dt * v;

        if (z > pi_real*2)
            z -= pi_real*2;
    }
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template<typename SystemType>
bool cmd_anim_diff(ParseCtx& ctx)
{
    AnimRenderer rndr(-3.5, 3.5, -4.5, 4.5);

    SystemType s;
    if (!peek(ctx, Token::Eof))
        s.setParamA(parse_expression(ctx));
    if (!peek(ctx, Token::Eof))
        s.setParamB(parse_expression(ctx));

    real_t step = 0.001;
    for (;;)
    {
        for (int i = 0; i < 10000; ++i)
        {
            s.next(step);

            const real_t x = s.getX();
            const real_t y = s.getY();

            const real_t xi = rndr.x(x);
            const real_t yi = rndr.y(y);

            rndr.safePlot(xi, yi);
        }
    
        rndr.blit();
        rndr.darken();

        if (rndr.check_for_break())
            break;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

template<typename SystemType>
bool cmd_anim_poincare(ParseCtx& ctx)
{
    AnimRenderer rndr(-3.5, 3.5, -4.5, 4.5);

    SystemType s;
    if (!peek(ctx, Token::Eof))
        s.setParamA(parse_expression(ctx));
    if (!peek(ctx, Token::Eof))
        s.setParamB(parse_expression(ctx));

    real_t step = 0.01;
    constexpr real_t slice = pi_real/2;
    bool phiBelowSlice = s.getPhi() < slice;

    for (int frame = 0; /**/; ++frame)
    {
        for (int i = 0; i < 250000; ++i)
        {
            s.next(step);

            if (phiBelowSlice && s.getPhi() >= slice)
            {
                const real_t x = s.getX();
                const real_t y = s.getY();

                const real_t xi = rndr.x(x);
                const real_t yi = rndr.y(y);

                rndr.safePlot(xi, yi);

                phiBelowSlice = false;
            }
            else if (!phiBelowSlice)
            {
                phiBelowSlice = (s.getPhi() < slice);
            }
        }

        rndr.blit();

        if ((frame & 7) == 0)
            rndr.darken();

        if (rndr.check_for_break())
            break;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

struct TinkerbellMap
{
    static real_t getMinX() { return -1.65; }
    static real_t getMinY() { return -1.65; }
    static real_t getMaxX() { return 1; }
    static real_t getMaxY() { return 1; }

    real_t a = 0.9;
    real_t b = -0.6013;
    real_t c = 2;
    real_t d = 0.49;
    
    void setParam(int ix, double val)
    {
        switch(ix)
        {
        case 0:     a = real_t(val);    return;
        case 1:     b = real_t(val);    return;
        case 2:     c = real_t(val);    return;
        case 3:     d = real_t(val);    return;
        }
    }
    void setParam(const char* name, double val)
    {
        switch (*name)
        {
        case 'a': case 'A':     a = real_t(val);    return;
        case 'b': case 'B':     b = real_t(val);    return;
        case 'c': case 'C':     c = real_t(val);    return;
        case 'd': case 'D':     d = real_t(val);    return;
        }
    }

    real_t x = 0.1;
    real_t y = 0.1;

    real_t getX() const { return x; }
    real_t getY() const { return y; }

    void next()
    {
        real_t xn = x*x - y*y + a*x + b*y;
        real_t yn = 2*x*y + c*x + d*y;

        x = xn;
        y = yn;
    }
};


//-------------------------------------------------------------------------------------------------

template<typename XYSystemType>
bool cmd_anim_xy(ParseCtx& ctx)
{
    using Sys = XYSystemType;
    AnimRenderer rndr(Sys::getMinX(), Sys::getMaxX(), Sys::getMinY(), Sys::getMaxY());

    XYSystemType s;
    int paramIx = 0;
    while (!peek(ctx, Token::Eof))
    {
        char sym[kMaxSymbolLength+1];
        if (peek(ctx, Token::Symbol))
        {
            if (!expect_symbol(ctx, sym))
                return false;
            if (!expect(ctx, Token::Equals))
                return false;
            double val = parse_primary(ctx);
            if (ctx.Error)
                return false;

            s.setParam(sym, val);
        }
        else
        {
            double val = parse_primary(ctx);
            if (ctx.Error)
                return false;

            s.setParam(paramIx, val);
            ++paramIx;
        }
    }

    // vary param d by +/- 10%
    const real_t originalD = s.d;
    const real_t scaleD = 0.01f * originalD;
    real_t t = 0;
    for (;; t += 0.0005f)
    {
        s.d = originalD + scaleD * sinf(t);

        for (int i = 0; i < 400; ++i)
        {
            s.next();

            const real_t x = s.getX();
            const real_t y = s.getY();

            const real_t xi = rndr.x(x);
            const real_t yi = rndr.y(y);

            rndr.safePlot(xi, yi);
        }
    
        rndr.blit();
        rndr.darken();

        if (rndr.check_for_break())
            break;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

void register_chaos_commands()
{
    register_calc_cmd(cmd_anim_diff<DampedPendulumSystem>, "dd", "d", "draw an animated diff eqn");
    register_calc_cmd(cmd_anim_poincare<DampedPendulumSystem>, "pd", "p", "draw an animated poincare slice of a diff eqn");
    register_calc_cmd(cmd_anim_diff<ForcedVdPolOscillator>, "df", "d", "draw an animated diff eqn");
    register_calc_cmd(cmd_anim_poincare<ForcedVdPolOscillator>, "pf", "p", "draw an animated poincare slice of a diff eqn");
    register_calc_cmd(cmd_anim_diff<SignumSystem>, "ds", "d", "draw an animated diff eqn");
    register_calc_cmd(cmd_anim_poincare<SignumSystem>, "ps", "p", "draw an animated poincare slice of a diff eqn");

    register_calc_cmd(cmd_anim_xy<TinkerbellMap>, "xy", "p", "draw a tinkerbell map");
}

//-------------------------------------------------------------------------------------------------

