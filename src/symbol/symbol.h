// src/symbol/symbol.h

#pragma once

#include "common.h"

struct SymbolTable;

typedef struct {
    size_t dim_count;
    size_t capacity;
    size_t** dims;
} ArrayData;

typedef struct {
    char* label_name;
    size_t param_count;
    size_t param_ptr;
    ValueType* param_types;
    size_t* param_dim_counts;           // Only non-zero for param_types that are arrays
    struct SymbolTable* scope;          // Scope belonging to this function
} FunData;

typedef struct {
    struct SymbolTable* scope;          // Create own scope for for-loops
} ForloopData;

typedef struct {
    SymbolType stype;
    ValueType vtype;
    char* name;
    size_t offset;                      // Offset within scope
    bool imported;                      // True for imported identifiers
    bool exported;                      // True for exported identifiers
    struct SymbolTable* parent_scope;   // Scope this symbol is assigned to
    union {
        ArrayData array;
        FunData fun;
        ForloopData forloop;
    } as;
} Symbol;

Symbol* SBfromFun(char* name, ValueType vt, size_t param_count, bool imported);
Symbol* SBfromArray(char* name, ValueType vt, bool imported);
Symbol* SBfromVar(char* name, ValueType vt, bool imported);
Symbol* SBfromForLoop(char* adjusted_name);
void SBfree(Symbol** s_ptr);
void SBaddDim(Symbol* s, size_t dim);
void SBaddParam(Symbol* s, size_t dim_count);
