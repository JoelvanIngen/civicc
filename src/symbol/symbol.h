// src/symbol/symbol.h

#pragma once

#include "common.h"

typedef enum {
    ST_VALUEVAR,
    ST_ARRAYVAR,
    ST_FUNCTION,
} SymbolType;

typedef enum {
    VT_NUM,
    VT_FLOAT,
    VT_BOOL,
    VT_VOID,
    VT_NUMARRAY,
    VT_FLOATARRAY,
    VT_BOOLARRAY,
} ValueType;

typedef struct {
    size_t dim_count;
    size_t capacity;
    size_t* dims;
} ArrayData;

typedef struct {
    size_t param_count;
    size_t capacity;
    ValueType* param_types;
} FunData;

typedef struct {
    SymbolType stype;
    ValueType vtype;
    const char* name;
    union {
        ArrayData array;
        FunData fun;
    } as;
} Symbol;

Symbol* SBfromFun(const char* name, ValueType vt);
Symbol* SBfromArray(const char* name, ValueType vt);
Symbol* SBfromVar(const char* name, ValueType vt);
void SBfree(Symbol** s);
void SBaddDim(Symbol* s, size_t dim);
void SBaddParam(Symbol* s, ValueType vt);
