// src/symbol/symbol.h

#pragma once

#include "common.h"

struct SymbolTable;

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
    VT_NULL                             // Used to set as "no value top" within the code
} ValueType;

typedef struct {
    size_t dim_count;
    size_t capacity;
    size_t** dims;
} ArrayData;

typedef struct {
    size_t param_count;
    size_t capacity;
    ValueType* param_types;
    size_t* param_dim_counts;           // Only non-zero for param_types that are arrays
    struct SymbolTable* scope;          // Scope belonging to this function
} FunData;

typedef struct {
    SymbolType stype;
    ValueType vtype;
    char* name;
    size_t offset;                      // Offset within scope
    struct SymbolTable* scope;          // Scope this symbol is assigned to
    union {
        ArrayData array;
        FunData fun;
    } as;
} Symbol;

Symbol* SBfromFun(char* name, ValueType vt);
Symbol* SBfromArray(char* name, ValueType vt);
Symbol* SBfromVar(char* name, ValueType vt);
void SBfree(Symbol** s_ptr);
void SBaddDim(Symbol* s, size_t dim);
void SBaddParam(Symbol* s, ValueType vt, size_t dim_count);
