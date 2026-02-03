#pragma once

//-----------------------------------------------------------------------------------------------

struct ParseCtx;

//-----------------------------------------------------------------------------------------------

bool eval_named_value(const char* name, double& outVal);

bool define_value(const char* name, double val, ParseCtx& ctx);
void undef_value(const char* name);

//-----------------------------------------------------------------------------------------------

