#pragma once

//-------------------------------------------------------------------------------------------------

struct ParseCtx;
struct UserFunction;

//-------------------------------------------------------------------------------------------------

bool eval_function(const char* name, double arg1, double& outVal, ParseCtx& ctx);

double eval_user_func(const UserFunction* func, double arg1, ParseCtx& ctx);

//-------------------------------------------------------------------------------------------------

bool define_function(const char* name, const char* arg, ParseCtx& ctx);

bool is_user_func(const char* name);
const UserFunction* lookup_user_func(const char* name);

//-------------------------------------------------------------------------------------------------

struct FunctionDef;
using BuiltinFunctionIt = const FunctionDef*;

struct UserFunction;
using UserFunctionIt = const UserFunction*;

BuiltinFunctionIt function_builtin_begin();
BuiltinFunctionIt function_next(BuiltinFunctionIt it);
const char* function_name(BuiltinFunctionIt it);

UserFunctionIt function_user_begin();
UserFunctionIt function_next(UserFunctionIt it);
const char* function_name(UserFunctionIt it);
const char* function_def(UserFunctionIt it);

//-------------------------------------------------------------------------------------------------

