#include "symbols.h"

#include "parser.h"

#include <cstring>

//-----------------------------------------------------------------------------------------------

constexpr int kMaxUserSymbols = 25;

//-----------------------------------------------------------------------------------------------

struct SymbolDef
{
    const char* Name = nullptr;
    double Value = 0.0;
};

struct UserSymbol
{
    char Name[kMaxSymbolLength+1] = {0};
    double Value = 0.0;

    bool IsUsed = false;
};

//-----------------------------------------------------------------------------------------------

SymbolDef gSymbols[] = 
{
    { .Name = "pi", .Value = 3.1415926535897932384626433 },
    { .Name = "e",  .Value = 2.7182818284590452353602874 },
};
constexpr int kNumSymbols = sizeof(gSymbols) / sizeof(gSymbols[0]);

UserSymbol gUserSymbols[kMaxUserSymbols];

//-----------------------------------------------------------------------------------------------

const SymbolDef* find_core_symbol(const char* name)
{
    for (const SymbolDef& sym : gSymbols)
    {
        if (strcmp(sym.Name, name) == 0)
        {
            return &sym;
        }
    }
    return nullptr;
}

bool eval_named_value(const char* name, double& outVal)
{
    if (const SymbolDef* sym = find_core_symbol(name))
    {
        outVal = sym->Value;
        return true;
    }

    for (const UserSymbol& sym : gUserSymbols)
    {
        if (sym.IsUsed && (strcmp(sym.Name, name) == 0))
        {
            outVal = sym.Value;
            return true;
        }
    }

    outVal = 0.0;
    return false;
}

//-----------------------------------------------------------------------------------------------

static UserSymbol* find_or_alloc_usersym(const char* name)
{
    UserSymbol* free_sym = nullptr;

    for (UserSymbol& sym : gUserSymbols)
    {
        if (!sym.IsUsed)
        {
            if (!free_sym)
                free_sym = &sym;
            continue;
        }

        if (strcmp(name, sym.Name) == 0)
            return &sym;
    }

    if (free_sym)
    {
        strcpy(free_sym->Name, name);
        free_sym->IsUsed = true;
        return free_sym;
    }

    return nullptr;
}

bool define_value(const char* name, double val, ParseCtx& ctx)
{
    if (find_core_symbol(name))
    {
        on_parse_error(ctx, "can't redefine a constant");
        return false;
    }

    UserSymbol* sym = find_or_alloc_usersym(name);
    if (!sym)
    {
        on_parse_error(ctx, "too many user symbols");
        return false;
    }

    sym->Value = val;
    return true;
}

void undef_value(const char* name)
{
    UserSymbol* sym = find_or_alloc_usersym(name);
    if (sym)
    {
        sym->IsUsed = false;
    }
}

//-----------------------------------------------------------------------------------------------

BuiltinSymbolIt symbol_builtin_begin()
{
    return gSymbols;
}

BuiltinSymbolIt symbol_next(BuiltinSymbolIt it)
{
    if (!it)
        return nullptr;

    ++it;
    if (it >= gSymbols + kNumSymbols)
        return nullptr;

    return it;
}

const char* symbol_name(BuiltinSymbolIt it)
{
    if (!it)
        return "<null>";

    return it->Name;
}

UserSymbolIt symbol_user_begin()
{
    UserSymbolIt it = gUserSymbols;

    if (!it->IsUsed)
        it = symbol_next(it);

    return it;
}

UserSymbolIt symbol_next(UserSymbolIt it)
{
    if (!it)
        return nullptr;

    for (++it; it < gUserSymbols + kMaxUserSymbols; ++it)
    {
        if (it->IsUsed)
            return it;
    }

    return nullptr;
}

const char* symbol_name(UserSymbolIt it)
{
    if (!it)
        return "<null>";

    return it->Name;
}

double symbol_val(UserSymbolIt it)
{
    if (!it)
        return 1.0 / 0.0;

    return it->Value;
}

//-----------------------------------------------------------------------------------------------
