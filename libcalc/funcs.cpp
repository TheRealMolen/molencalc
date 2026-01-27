#include "funcs.h"

#include <cmath>
#include <cstring>


//-----------------------------------------------------------------------------------------------

typedef double (*CalcDoubleFn)(double);

// a function is defined strictly as taking zero or more args and returning a single value
// TODO: allow vector/matrix return vals
struct FunctionDef
{
    const char* Name = nullptr;
    const char* Args = "d";
    CalcDoubleFn FuncPtr = nullptr;
};

//-----------------------------------------------------------------------------------------------

FunctionDef gFunctions[] =
{
    { .Name = "sin", .FuncPtr = (CalcDoubleFn)sin },
    { .Name = "cos", .FuncPtr = (CalcDoubleFn)cos },
    { .Name = "tan", .FuncPtr = (CalcDoubleFn)tan },

    { .Name = "asin", .FuncPtr = (CalcDoubleFn)asin },
    { .Name = "acos", .FuncPtr = (CalcDoubleFn)acos },
    { .Name = "atan", .FuncPtr = (CalcDoubleFn)atan },

    { .Name = "ln", .FuncPtr = (CalcDoubleFn)log },
    { .Name = "log", .FuncPtr = (CalcDoubleFn)log10 },
    { .Name = "sqrt", .FuncPtr = (CalcDoubleFn)sqrt },
};
constexpr int kNumFunctions = sizeof(gFunctions) / sizeof(gFunctions[0]);

//-----------------------------------------------------------------------------------------------

bool eval_function(const char* name, double arg1, double& outVal)
{
    for (const FunctionDef& func : gFunctions)
    {
        if (strcmp(func.Name, name) == 0)
        {
            outVal = func.FuncPtr(arg1);
            return true;
        }
    }

    outVal = 0.0;
    return false;

}

//-----------------------------------------------------------------------------------------------


