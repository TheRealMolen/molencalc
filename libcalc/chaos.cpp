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

void register_chaos_commands()
{
    register_calc_cmd(cmd_anim_diff<DampedPendulumSystem>, "dd", "d", "draw an animated diff eqn");
    register_calc_cmd(cmd_anim_poincare<DampedPendulumSystem>, "pd", "p", "draw an animated poincare...\n slice of a diff eqn");
    register_calc_cmd(cmd_anim_diff<ForcedVdPolOscillator>, "df", "d", "draw an animated diff eqn");
    register_calc_cmd(cmd_anim_poincare<ForcedVdPolOscillator>, "pf", "p", "draw an animated poincare...\n slice of a diff eqn");
    register_calc_cmd(cmd_anim_diff<SignumSystem>, "ds", "d", "draw an animated diff eqn");
    register_calc_cmd(cmd_anim_poincare<SignumSystem>, "ps", "p", "draw an animated poincare...\n slice of a diff eqn");
}

//-------------------------------------------------------------------------------------------------

