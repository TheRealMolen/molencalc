#pragma once

//-----------------------------------------------------------------------------------------------

struct ParseCtx;

//-----------------------------------------------------------------------------------------------

bool eval_named_value(const char* name, double& outVal);

bool define_value(const char* name, double val, ParseCtx& ctx);
void undef_value(const char* name);

//-----------------------------------------------------------------------------------------------

struct SymbolDef;
using BuiltinSymbolIt = const SymbolDef*;

struct UserSymbol;
using UserSymbolIt = const UserSymbol*;

BuiltinSymbolIt symbol_builtin_begin();
BuiltinSymbolIt symbol_next(BuiltinSymbolIt it);
const char* symbol_name(BuiltinSymbolIt it);

UserSymbolIt symbol_user_begin();
UserSymbolIt symbol_next(UserSymbolIt it);
const char* symbol_name(UserSymbolIt it);
double symbol_val(UserSymbolIt it);

//-----------------------------------------------------------------------------------------------

